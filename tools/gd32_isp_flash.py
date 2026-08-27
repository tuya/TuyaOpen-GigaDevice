#!/usr/bin/env python3
# coding=utf-8

"""
Serial flashing for GD32VW553, without the Windows tool.

gd32_mass_flash.py drives GD32MassProductionTool_CMD.exe, which enumerates
ports through WMI and addresses them as COMx - neither exists on Linux. This is
the same two-stage flow, spoken directly over pyserial:

1. ROM bootloader ISP (57600 8E1, BOOT0 high at reset). A GD variant of the ST
   bootloader protocol: 0x7F handshake, 0x31 write-memory, 0x21 go. Used to
   place the vendor's xmodem loader in SRAM at 0x20002000 and jump to it.
2. The loader's own protocol (8N1): a sync byte, then single-byte commands for
   chip id, flash erase, baudrate and frame size, followed by an xmodem-1K
   variant whose blocks carry a 4-byte destination address, and finally a
   SHA-256 check of what landed in flash.

The loader binary is the vendor's own - tools/GD32MassProductionTool_v1.0.5g/
xmodem/GD32VW553_Xmodem.bin, shipped in the tool archive - so nothing here has
to reimplement the flash writing itself.

flash_firmware() mirrors gd32_mass_flash.flash_firmware(), so
platform_flash_bridge.py can pick either at runtime.
"""

from __future__ import annotations

import hashlib
import logging
import os
import time
from pathlib import Path
from typing import Any, Dict, Optional

try:
    import serial
except ImportError:  # reported properly by flash_firmware()
    serial = None

TOOLS_DIR = Path(__file__).resolve().parent
TOOL_PACKAGE_DIRNAME = "GD32MassProductionTool_v1.0.5g"
SRAM_LOADERS = {
    "GD32VW553": "GD32VW553_Xmodem.bin",
    "GD32VW553H": "GD32VW553_Xmodem.bin",
    "GD32W515": "GD32W515_Xmodem.bin",
}

# ROM bootloader (stage 1)
ISP_BAUD = 57600
ISP_INIT = 0x7F
ISP_ACK = 0x79
ISP_NAK = 0x1F
ISP_CMD_WRITE = 0x31
ISP_CMD_GO = 0x21
SRAM_ADDR = 0x20002000
SRAM_PAGE = 240

# SRAM loader (stage 2)
LOADER_SYNC = b"u"
SYNC_ACK = 0x79
SYNC_NAK = 0x1F
SOH, STX, EOT, ACK, CAN = 0x01, 0x02, 0x04, 0x06, 0x18
PAD = 0xFF
CMD_BAUDSET = 0x05
CMD_FRMSIZE = 0x07
CMD_FLERASE = 0x17
CMD_CHIPGET = 0x20
CMD_IMGCHCK = 0x21
CMD_IMGDONE = 0x22
# trailing byte of an IMGCHCK frame: which digest the loader should return
HASH_KIND_SHA256 = 0x02

LOADER_BAUD_INIT = 115200
DEFAULT_BAUD = 2000000
# The loader's frame size is negotiated per baudrate; these are the tool's.
TX_BLOCK_BY_BAUD = {921600: 2048, 2000000: 2560, 3000000: 3072}
TX_BLOCK_DEFAULT = 1024
FLASH_BASE = 0x08000000
SECTOR = 0x1000
ERASE_TIMEOUT = 120.0
# How long to keep offering the handshake while waiting for a reset. Long
# enough that a person can reach the buttons; raise it via the environment when
# driving the flash from somewhere that cannot be watched in real time.
ISP_SYNC_WINDOW = float(os.environ.get("GD32_ISP_SYNC_WINDOW", "30"))
# a board already in ISP answers at once; this is just that check
ISP_FIRST_TRY = 1.0


class FlashError(Exception):
    """Anything that stops the flash; the message is user-facing."""


def _log(logger: Optional[logging.Logger], level: str, msg: str) -> None:
    if logger:
        getattr(logger, level)(msg)
    else:
        print(msg)


def _checksum(data: bytes, seed: int = 0) -> int:
    """8-bit sum, as the loader's frames use."""
    total = seed
    for b in data:
        total = (total + b) & 0xFF
    return total


def _xor(data: bytes, seed: int = 0) -> int:
    """8-bit xor, as the ROM bootloader's frames use."""
    value = seed
    for b in data:
        value ^= b
    return value & 0xFF


class _Link:
    """Serial port with the read-exactly / expect-byte helpers both stages need."""

    def __init__(self, port: str, baud: int, parity, logger=None):
        self.name = port
        self.logger = logger
        # Leave the modem lines alone: on a CH340 board they can be wired to
        # reset, and a reset here drops the chip out of ISP mode - BOOT0 is a
        # button the user has already let go of by the time we open the port.
        self.ser = serial.Serial(port, baud, bytesize=8, parity=parity,
                                 stopbits=1, timeout=1, dsrdtr=False,
                                 rtscts=False)
        self.ser.dtr = False
        self.ser.rts = False
        self.ser.reset_input_buffer()
        self.ser.reset_output_buffer()

    def close(self):
        try:
            self.ser.close()
        except Exception:
            pass

    def reopen(self, baud: int, parity) -> None:
        """Change line settings in place.

        Closing and reopening would toggle DTR/RTS, which on this board reads
        as a reset - and a reset with BOOT0 released leaves ISP for good.
        """
        if self.ser.baudrate == baud and self.ser.parity == parity:
            return
        self.ser.baudrate = baud
        self.ser.parity = parity
        time.sleep(0.02)
        self.ser.reset_input_buffer()
        self.ser.reset_output_buffer()

    def write(self, data: bytes) -> None:
        self.ser.write(data)
        self.ser.flush()

    def read_exact(self, count: int, timeout: float) -> bytes:
        self.ser.timeout = timeout
        got = self.ser.read(count)
        return got

    def expect(self, value: int, what: str, timeout: float = 5.0) -> None:
        got = self.read_exact(1, timeout)
        if not got:
            raise FlashError(f"{what}: timed out waiting for a reply")
        if got[0] != value:
            raise FlashError(
                f"{what}: got 0x{got[0]:02X}, expected 0x{value:02X}")


class RomIsp:
    """Stage 1: the chip's ROM bootloader."""

    def __init__(self, link: _Link, logger=None):
        self.link = link
        self.logger = logger

    def sync(self, window: float = ISP_SYNC_WINDOW, notify=None) -> bool:
        """Poll for the ROM bootloader until `window` seconds have passed.

        The bootloader only listens for the handshake for a short spell after
        reset before it hands over to the application, and there is no way to
        press BOOT0 and reset in step with a script. So keep the 0x7F going and
        let the user reset the board into it whenever they are ready.
        """
        deadline = time.time() + window
        announced = False
        while time.time() < deadline:
            self.link.ser.reset_input_buffer()
            self.link.write(bytes([ISP_INIT]))
            time.sleep(0.05)
            got = self.link.read_exact(1, 0.1)
            # a stray 0x00 shows up on a noisy line; the real reply follows it
            if got and got[0] == 0:
                got = self.link.read_exact(1, 0.1)
            if got and got[0] in (ISP_ACK, ISP_NAK):
                return True
            if notify and not announced:
                notify()
                announced = True
        return False

    def write_memory(self, addr: int, data: bytes) -> None:
        self.link.write(bytes([ISP_CMD_WRITE, 0xCE]))
        self.link.expect(ISP_ACK, "write command", 50.0)

        addr_be = addr.to_bytes(4, "big")
        self.link.write(addr_be + bytes([_xor(addr_be)]))
        self.link.expect(ISP_ACK, f"write address 0x{addr:08X}", 50.0)

        # the bootloader takes whole words and counts them as len-1
        size = (len(data) + 3) & ~3
        payload = data + bytes([PAD]) * (size - len(data))
        self.link.write(bytes([size - 1]) + payload +
                        bytes([_xor(payload, size - 1)]))
        time.sleep(0.04)
        self.link.expect(ISP_ACK, "write data", 50.0)

    def go(self, addr: int) -> None:
        self.link.write(bytes([ISP_CMD_GO, 0xDE]))
        self.link.expect(ISP_ACK, "go command", 5.0)
        addr_be = addr.to_bytes(4, "big")
        self.link.write(addr_be + bytes([_xor(addr_be)]))
        self.link.expect(ISP_ACK, f"go address 0x{addr:08X}", 50.0)


class SramLoader:
    """Stage 2: the vendor loader now running from SRAM."""

    def __init__(self, link: _Link, logger=None):
        self.link = link
        self.logger = logger
        self.packetno = 1
        self.tx_block = TX_BLOCK_DEFAULT

    def sync(self, attempts: int = 10) -> bool:
        for _ in range(attempts):
            self.link.ser.reset_input_buffer()
            self.link.write(LOADER_SYNC)
            got = self.link.read_exact(1, 0.3)
            if got and got[0] in (SYNC_ACK, SYNC_NAK):
                return True
        return False

    def chip_id(self) -> bytes:
        self.link.write(bytes([CMD_CHIPGET]))
        got = self.link.read_exact(4, 5.0)
        if len(got) != 4:
            raise FlashError("chip id: short reply")
        return got

    def erase(self, addr: int, length: int, chip_erase: bool) -> None:
        sectors = (length + SECTOR - 1) // SECTOR
        frame = bytes([
            CMD_FLERASE,
            1 if chip_erase else 0,
            addr & 0xFF, (addr >> 8) & 0xFF, (addr >> 16) & 0xFF,
            sectors & 0xFF, (sectors >> 8) & 0xFF,
        ])
        self.link.write(frame)
        self.link.expect(ACK, "flash erase", ERASE_TIMEOUT)

    def set_baudrate(self, baud: int) -> None:
        """Ask for a new baudrate, then follow the target onto it.

        The ack still comes at the old rate - the vendor tool reopens its port
        only afterwards - so reading it before switching is what keeps the two
        sides in step.
        """
        self.link.write(bytes([CMD_BAUDSET]) + baud.to_bytes(4, "little"))
        self.link.expect(ACK, f"baudrate {baud}", 5.0)
        time.sleep(0.05)
        self.link.reopen(baud, serial.PARITY_NONE)

    def set_frame_size(self, size: int) -> None:
        self.tx_block = size
        self.link.write(bytes([CMD_FRMSIZE, (size >> 8) & 0xFF]))
        self.link.expect(ACK, f"frame size {size}", 5.0)

    def send_block(self, data: bytes, addr: int) -> None:
        """One xmodem block: header, destination address, payload, checksum."""
        frame = bytearray([PAD]) * (self.tx_block + 8)
        frame[0] = STX if self.tx_block > 128 else SOH
        frame[1] = self.packetno
        frame[2] = (~self.packetno) & 0xFF
        frame[3:7] = addr.to_bytes(4, "little")
        frame[7:7 + len(data)] = data
        frame[7 + self.tx_block] = _checksum(bytes(frame[3:3 + 4 + self.tx_block]))

        for attempt in range(6):
            self.link.write(bytes(frame))
            got = self.link.read_exact(1, 5.0)
            if got and got[0] == ACK:
                self.packetno = (self.packetno + 1) & 0xFF
                return
            if got and got[0] == CAN:
                self.link.write(bytes([ACK]))
                raise FlashError(f"block at 0x{addr:08X}: cancelled by target")
            _log(self.logger, "warning",
                 f"block at 0x{addr:08X}: retry {attempt + 1}/6")
        raise FlashError(f"block at 0x{addr:08X}: no ack after 6 tries")

    def finish(self) -> None:
        self.link.write(bytes([EOT]))
        self.link.expect(ACK, "end of transfer", 5.0)

    def image_hash(self, addr: int, size: int) -> bytes:
        aligned = (size + 3) & ~3
        frame = bytes([
            CMD_IMGCHCK,
            addr & 0xFF, (addr >> 8) & 0xFF, (addr >> 16) & 0xFF,
            aligned & 0xFF, (aligned >> 8) & 0xFF, (aligned >> 16) & 0xFF,
            HASH_KIND_SHA256,
        ])
        self.link.write(frame)
        got = self.link.read_exact(32, 30.0)
        if len(got) != 32:
            raise FlashError("image check: short hash reply")
        return got

    def image_done(self, ok: bool) -> None:
        self.link.write(bytes([CMD_IMGDONE, 1 if ok else 0]))


def _expected_hash(image: bytes) -> bytes:
    """SHA-256 over the image padded to a word boundary, as the loader hashes it."""
    aligned = (len(image) + 3) & ~3
    return hashlib.sha256(image + bytes([PAD]) * (aligned - len(image))).digest()


def _loader_path(device: str) -> Path:
    name = SRAM_LOADERS.get(str(device).upper())
    if not name:
        raise FlashError(f"No SRAM loader known for device '{device}'.")
    path = TOOLS_DIR / TOOL_PACKAGE_DIRNAME / "xmodem" / name
    if not path.is_file():
        raise FlashError(
            f"SRAM loader not found: {path}\n"
            "It ships in GD32MassProductionTool_v1.0.5g.zip; run a flash once "
            "on Windows or unzip the archive next to it.")
    return path


def flash_firmware(
    device: Optional[str],
    port: Optional[str],
    baud: Optional[Any],
    start: Optional[str],
    binfile: Optional[str],
    logger: Optional[logging.Logger] = None,
    **kwargs: Any,
) -> Dict[str, Any]:
    """Flash one image. Signature matches gd32_mass_flash.flash_firmware()."""
    result: Dict[str, Any] = {
        "success": False,
        "message": "",
        "stage": "init",
        "device": device,
        "port": port,
        "baud": baud,
        "start": start,
        "binfile": binfile,
    }

    if serial is None:
        result["message"] = "pyserial is required: pip install pyserial"
        return result
    if not port:
        result["message"] = "No serial port given."
        return result
    if not binfile or not os.path.isfile(binfile):
        result["message"] = f"Firmware not found: {binfile}"
        return result

    try:
        baudrate = int(baud) if baud else DEFAULT_BAUD
    except (TypeError, ValueError):
        result["message"] = f"Invalid baud: {baud}"
        return result

    flash_addr = int(str(start), 0) if start else FLASH_BASE
    image = Path(binfile).read_bytes()
    link = None

    try:
        loader_fw = _loader_path(device)

        # Order matters. The ROM bootloader has to be greeted first, with
        # nothing but 0x7F: a board already sitting in ISP takes the loader
        # probe's sync byte as garbage and stops answering the handshake
        # afterwards. 0x7F costs nothing on a board that is not in ISP, so
        # trying it first is free - and it keeps "press the buttons, then run
        # the command" working, which is how the vendor tool behaves.
        result["stage"] = "isp"
        link = _Link(port, ISP_BAUD, serial.PARITY_EVEN, logger)
        isp = RomIsp(link, logger)
        loader = SramLoader(link, logger)
        in_isp = isp.sync(window=ISP_FIRST_TRY)

        already_running = False
        if not in_isp:
            # No ROM bootloader: either nothing is listening yet, or a loader
            # from an earlier flash is still running - the vendor tool calls
            # skipping the preload skip_preload. Whatever rate that loader was
            # last switched to is worth a try.
            result["stage"] = "loader"
            for probe in (LOADER_BAUD_INIT, baudrate):
                link.reopen(probe, serial.PARITY_NONE)
                if loader.sync(attempts=2):
                    already_running = True
                    _log(logger, "info", f"SRAM loader already running at {probe}")
                    break

        if not already_running:
            result["stage"] = "isp"
            link.reopen(ISP_BAUD, serial.PARITY_EVEN)
            waiting = lambda: _log(  # noqa: E731 - one-liner notice, not logic
                logger, "info",
                "waiting for the board: hold BOOT0, tap RESET, release both "
                f"(up to {int(ISP_SYNC_WINDOW)}s)")
            if not in_isp and not isp.sync(notify=waiting):
                raise FlashError(
                    "No reply from the ROM bootloader within "
                    f"{int(ISP_SYNC_WINDOW)}s. BOOT0 has to be high when reset "
                    "is released.")
            _log(logger, "info", "ISP handshake ok")

            result["stage"] = "preload"
            blob = loader_fw.read_bytes()
            for off in range(0, len(blob), SRAM_PAGE):
                isp.write_memory(SRAM_ADDR + off, blob[off:off + SRAM_PAGE])
            isp.go(SRAM_ADDR)
            _log(logger, "info",
                 f"SRAM loader running ({len(blob)} bytes at 0x{SRAM_ADDR:08X})")

            result["stage"] = "loader"
            link.reopen(LOADER_BAUD_INIT, serial.PARITY_NONE)
            if not loader.sync():
                raise FlashError("SRAM loader did not answer its sync byte.")
        chip = loader.chip_id()
        result["chip_id"] = chip.hex().upper()
        _log(logger, "info", f"chip id {chip.hex().upper()}")

        result["stage"] = "erase"
        _log(logger, "info", "erasing flash ...")
        loader.erase(flash_addr, len(image), chip_erase=True)

        result["stage"] = "download"
        loader.set_baudrate(baudrate)
        loader.set_frame_size(TX_BLOCK_BY_BAUD.get(baudrate, TX_BLOCK_DEFAULT))
        _log(logger, "info",
             f"downloading {len(image)} bytes at {baudrate} baud, "
             f"block {loader.tx_block}")

        started = time.time()
        last_pct = -1
        for off in range(0, len(image), loader.tx_block):
            loader.send_block(image[off:off + loader.tx_block],
                              flash_addr + off)
            pct = (off + loader.tx_block) * 100 // len(image)
            if pct != last_pct and pct % 10 == 0:
                _log(logger, "info", f"  {min(pct, 100)}%")
                last_pct = pct
        loader.finish()
        elapsed = time.time() - started
        _log(logger, "info", f"download done in {elapsed:.1f}s")

        result["stage"] = "verify"
        got = loader.image_hash(flash_addr, len(image))
        want = _expected_hash(image)
        ok = got == want
        loader.image_done(ok)
        if not ok:
            raise FlashError(
                f"Verify failed: flash hash {got.hex()} != image {want.hex()}")
        _log(logger, "info", "verify ok")

        result["stage"] = "done"
        result["success"] = True
        result["message"] = f"Download succeeded in {elapsed:.1f}s."
    except FlashError as exc:
        result["message"] = str(exc)
    except Exception as exc:  # serial errors and anything unforeseen
        result["message"] = f"{type(exc).__name__}: {exc}"
    finally:
        if link:
            link.close()

    return result
