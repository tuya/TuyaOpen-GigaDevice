#!/usr/bin/env python3
# coding=utf-8

"""
Platform flash bridge for GD32.

Entry point for tos.py flash: platform_flash().
"""

from __future__ import annotations

import argparse
import importlib.util
import json
import logging
import os
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional

try:
    from serial.tools import list_ports
except ImportError:
    list_ports = None

GD32_FLASH_DEVICE = "GD32VW553H"
_CHIP_TO_DEVICE = {
    "GD32VW553": GD32_FLASH_DEVICE,
}


def _parse_cfg_file(config_path: str) -> dict:
    data = {}
    if not os.path.isfile(config_path):
        return data
    with open(config_path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line.startswith("CONFIG_") or "=" not in line:
                continue
            key, value = line.split("=", 1)
            value = value.strip()
            if value.startswith('"'):
                data[key] = value.strip('"')
            elif value == "y":
                data[key] = True
            elif value.isdigit():
                data[key] = int(value)
            else:
                data[key] = value
    return data


def _get_board_flash_cfg(using_data: dict, boards_root: str) -> dict:
    platform = using_data.get("CONFIG_PLATFORM_CHOICE", "")
    board = using_data.get("CONFIG_BOARD_CHOICE", "")
    if not platform or not board:
        return {}
    config_file = os.path.join(boards_root, platform, board, "flash.cfg")
    return _parse_cfg_file(config_file)


def _resolve_device(using_data: dict, flash_cfg: dict) -> str:
    device = flash_cfg.get("CONFIG_FLASH_DEVICE", "")
    if device:
        return str(device)

    chip = using_data.get("CONFIG_CHIP_CHOICE", "")
    if chip:
        mapped = _CHIP_TO_DEVICE.get(str(chip).upper())
        if mapped:
            return mapped
        return str(chip)

    return using_data.get("CONFIG_PLATFORM_CHOICE", "")


def _resolve_baud(cli_baud: int, flash_cfg: dict) -> Optional[int]:
    if cli_baud:
        return cli_baud
    baud = flash_cfg.get("CONFIG_FLASH_BAUDRATE", 0)
    return baud if baud else None


def _list_serial_ports() -> List[str]:
    if list_ports is None:
        return []

    ports = []
    for item in list_ports.comports():
        device = item.device
        if device.startswith("/dev/ttyS"):
            continue
        ports.append(device)
    ports.sort()
    return ports


def choose_port(logger: Optional[logging.Logger] = None) -> Optional[str]:
    """
    Pick a serial port for flashing.

    - One port: use it directly.
    - Multiple ports: interactive selection.
    - No port: return None.
    """
    port_items = _list_serial_ports()
    if not port_items:
        return None

    if len(port_items) == 1:
        port = port_items[0]
        if logger:
            logger.info(f"Use serial port: {port}")
        else:
            print(f"Use serial port: {port}")
        return port

    if logger:
        logger.info("Multiple serial ports detected, please choose one:")
    else:
        print("Multiple serial ports detected, please choose one:")
    print("--------------------")
    for idx, name in enumerate(port_items, start=1):
        print(f"{idx}. {name}")
    print("--------------------")

    while True:
        try:
            raw = input("Select serial port: ").strip()
            num = int(raw)
            if 1 <= num <= len(port_items):
                port = port_items[num - 1]
                if logger:
                    logger.info(f"Use serial port: {port}")
                return port
        except ValueError:
            if logger:
                logger.warning("Invalid input, enter a number from the list.")
            else:
                print("Invalid input, enter a number from the list.")
        except KeyboardInterrupt:
            if logger:
                logger.warning("Port selection cancelled.")
            return None


def _resolve_port(cli_port: str, logger: Optional[logging.Logger] = None) -> Optional[str]:
    if cli_port and str(cli_port).strip():
        return str(cli_port).strip()
    return choose_port(logger)


def _normalize_device(device: Optional[str]) -> str:
    if not device or not str(device).strip():
        raise ValueError("Parameter 'device' cannot be empty.")
    key = str(device).strip().upper()
    return _CHIP_TO_DEVICE.get(key, str(device).strip())


def _load_flash_module():
    """Pick the flashing back end for this host.

    Windows drives GD32MassProductionTool_CMD.exe. That tool enumerates ports
    through WMI and addresses them as COMx, so everywhere else uses
    gd32_isp_flash.py, which speaks the same ROM-ISP and xmodem protocols over
    pyserial and loads the tool's own SRAM loader.
    """
    name = "gd32_mass_flash" if os.name == "nt" else "gd32_isp_flash"
    script_path = Path(__file__).resolve().parent / "tools" / f"{name}.py"
    if not script_path.is_file():
        raise FileNotFoundError(f"Cannot find {name}.py: {script_path}")

    spec = importlib.util.spec_from_file_location(name, script_path)
    if spec is None or spec.loader is None:
        raise ImportError(f"Cannot load module: {script_path}")

    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    if not callable(getattr(module, "flash_firmware", None)):
        raise ImportError(f"{name}.py has no flash_firmware()")
    return module


def flash_firmware(
    device: Optional[str],
    port: Optional[str],
    baud: Optional[Any],
    start: Optional[str],
    binfile: Optional[str],
    logger: Optional[logging.Logger] = None,
    **kwargs: Any,
) -> Dict[str, Any]:
    """Low-level flash API used by platform_flash() and CLI."""
    normalized = _normalize_device(device)
    flash_module = _load_flash_module()
    return flash_module.flash_firmware(
        device=normalized,
        port=port,
        baud=baud,
        start=start,
        binfile=binfile,
        logger=logger,
        **kwargs,
    )


def platform_flash(using_data, binfile, port, baud, boards_root, logger):
    """
    Platform flash entry called from tos.py flash.

    Reads board flash.cfg, resolves parameters, then flashes firmware.
    """
    flash_cfg = _get_board_flash_cfg(using_data, boards_root)
    device = _resolve_device(using_data, flash_cfg)
    baudrate = _resolve_baud(baud, flash_cfg)
    resolved_port = _resolve_port(port, logger)

    if not resolved_port:
        return {
            "success": False,
            "message": "No serial port available. Connect device or use -p/--port.",
        }

    if logger:
        logger.info(
            f"Flash params: device={device}, port={resolved_port}, "
            f"baud={baudrate or 'default'}, bin={binfile}")

    try:
        return flash_firmware(
            device=device,
            port=resolved_port,
            baud=baudrate,
            start=None,
            binfile=binfile,
            logger=logger,
        )
    except Exception as exc:
        return {
            "success": False,
            "message": str(exc),
        }


def _build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="GD32 platform flash bridge")
    parser.add_argument("--device", required=True, help="Device or chip name")
    parser.add_argument(
        "--port",
        default="",
        help="COM port, e.g. COM3 (auto-detect if omitted)",
    )
    parser.add_argument("--baud", default=None, help="Download baudrate")
    parser.add_argument("--binfile", required=True, help="Firmware bin path")
    return parser


def main() -> int:
    args = _build_arg_parser().parse_args()
    port = _resolve_port(args.port)
    if not port:
        print("No serial port available.")
        return 1
    result = flash_firmware(
        device=args.device,
        port=port,
        baud=args.baud,
        start=None,
        binfile=args.binfile,
        logger=None,
    )
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 0 if result.get("success") else 1


if __name__ == "__main__":
    raise SystemExit(main())
