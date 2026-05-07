#!/usr/bin/env python3
# coding=utf-8

"""
GD32 mass production flashing wrapper.

This script wraps GD32MassProductionTool_CMD.exe with a stable Python API:
1) scan and validate that the requested COM port is detected
2) choose COM selection mode (see ``DEFAULT_COM_SELECT_MODE``):
   - ``ignore``: ignore <other_coms ...>, then download (no specify)
   - ``specify``: specify <port>, then download (no ignore list)
3) download <chip> <baudrate> without_gd32f303 <binfile> page_erase
4) cleanup (always, even on exception): ``ignore nothing`` or ``specify nothing``
   matching the selected mode

API result fields:
- ``scan``: subprocess result for the ``scan`` step, or None if not reached
- ``ignore``: subprocess result for the ``ignore <coms>`` step, None if skipped
- ``download``: dict with ``specify`` and ``download`` subprocess details
  (inner ``download`` is None until the programming step runs), or None if
  no steps executed
- ``cleanup``: dict with subprocess results from the finally-block reset;
  exactly one of ``ignore`` or ``specify`` keys is set according to
  ``com_select_mode``
"""

from __future__ import annotations

import argparse
import hashlib
import json
import locale
import logging
import os
import re
import shlex
import shutil
import subprocess
import sys
import threading
import traceback
from pathlib import Path
from typing import Any, Dict, List, Optional


TOOLS_DIR = Path(__file__).resolve().parent

DEFAULT_BAUDRATE = "2000000"
DEFAULT_WITH_JTAG = "without_gd32f303"
DEFAULT_ERASE_OPTION = "page_erase"
TOOL_PACKAGE_DIRNAME = "GD32MassProductionTool_v1.0.5g"
TOOL_ARCHIVE_NAME = "GD32MassProductionTool_v1.0.5g.zip"
EXPECTED_CMD_SHA256 = "0DDAE19A3DA03E795623F8D8DDE1D322058D443CFCFCF52A3787218C15FF3095"
SUPPORTED_DEVICE_TO_CHIP = {
    "GD32VW553H": "GD32VW553",
}

# COM targeting: ignore all ports except the target via ``ignore``, or use ``specify``.
COM_SELECT_IGNORE = "ignore"
COM_SELECT_SPECIFY = "specify"
DEFAULT_COM_SELECT_MODE = COM_SELECT_SPECIFY

_PROGRESS_LINE_RE = re.compile(r"^\[(?P<port>COM\d+)\]\s+(?P<percent>\d{1,3})%")
_PROGRESS_LOG_STEP_PERCENT = 10


def _normalize_com_select_mode(mode: Optional[Any]) -> str:
    """
    Normalize COM selection mode string.

    Accepts case-insensitive ``ignore`` / ``specify``.
    """
    if mode is None or not str(mode).strip():
        return DEFAULT_COM_SELECT_MODE
    key = str(mode).strip().lower()
    if key == COM_SELECT_IGNORE:
        return COM_SELECT_IGNORE
    if key == COM_SELECT_SPECIFY:
        return COM_SELECT_SPECIFY
    raise ValueError(
        f"Invalid com_select_mode {mode!r}. "
        f"Use {COM_SELECT_IGNORE!r} or {COM_SELECT_SPECIFY!r}."
    )


def _format_cmd_for_log(cmd: list[str]) -> str:
    """Format argv for logs (paths with spaces safe on Windows)."""
    if sys.platform == "win32":
        return subprocess.list2cmdline(cmd)
    return shlex.join(cmd)


def _subprocess_text_encoding() -> str:
    """
    Encoding for decoding native tool stdout/stderr.

    Non-Windows: UTF-8. Windows: preferred ANSI/codepage (e.g. cp936) so localized
    tool output decodes without mojibake; unknown fallback UTF-8.
    """
    if sys.platform != "win32":
        return "utf-8"
    enc = locale.getpreferredencoding(False)
    if enc:
        return enc
    return "utf-8"


def _log(
    logger: Optional[logging.Logger],
    level: str,
    message: str,
) -> None:
    """Log via user logger first, fallback to stderr."""
    if logger is None:
        if level.lower() == "debug":
            return
        print(f"[{level.upper()}] {message}", file=sys.stderr)
        return

    log_fn = getattr(logger, level, None)
    if callable(log_fn):
        log_fn(message)
    else:
        logger.info(message)


def _should_log_tool_line(text: str, progress_buckets: Dict[str, int]) -> bool:
    """Return whether a native tool output line should be forwarded to logs."""
    progress_match = _PROGRESS_LINE_RE.match(text)
    if progress_match is None:
        return True

    try:
        percent = int(progress_match.group("percent"), 10)
    except ValueError:
        return True

    if percent < 0 or percent > 100:
        return True

    if percent != 100 and percent % _PROGRESS_LOG_STEP_PERCENT != 0:
        return False

    port = progress_match.group("port").upper()
    bucket = percent // _PROGRESS_LOG_STEP_PERCENT
    last_bucket = progress_buckets.get(port)
    if last_bucket is None or bucket != last_bucket:
        progress_buckets[port] = bucket
        return True

    return False


def _stderr_line_level(text: str) -> str:
    """
    Choose log level for one stderr line.

    Many native tools print normal progress to stderr. Only elevate likely errors.
    """
    lowered = text.lower()
    if any(keyword in lowered for keyword in ("error", "fail", "fatal", "exception")):
        return "warning"
    return "info"


def _excerpt_text(text: str, max_chars: int = 400) -> str:
    """Build one-line excerpt for result message/log."""
    clean = " ".join((text or "").split())
    if len(clean) <= max_chars:
        return clean
    return f"{clean[:max_chars]}..."


def _result_error_excerpt(step_result: Dict[str, Any]) -> str:
    """Get a concise error excerpt from stderr first, then stdout."""
    stderr_text = _excerpt_text(str(step_result.get("stderr") or ""))
    if stderr_text:
        return stderr_text
    return _excerpt_text(str(step_result.get("stdout") or ""))


def _run_cmd(
    cmd: list[str],
    cwd: Path,
    logger: Optional[logging.Logger],
) -> Dict[str, Any]:
    """Run one command and collect process details."""
    _log(logger, "info", f"Run command: {_format_cmd_for_log(cmd)}")
    enc = _subprocess_text_encoding()
    process = subprocess.Popen(
        cmd,
        cwd=str(cwd),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        encoding=enc,
        errors="replace",
        bufsize=1,
    )
    stdout_chunks: List[str] = []
    stderr_chunks: List[str] = []

    def _reader(
        pipe: Any,
        chunks: List[str],
        level: str,
        stream_name: str,
        dynamic_level: bool = False,
    ) -> None:
        progress_buckets: Dict[str, int] = {}
        try:
            for line in iter(pipe.readline, ""):
                chunks.append(line)
                text = line.rstrip("\r\n")
                if text and _should_log_tool_line(text, progress_buckets):
                    line_level = _stderr_line_level(text) if dynamic_level else level
                    _log(logger, line_level, f"{stream_name}: {text}")
        finally:
            pipe.close()

    threads: List[threading.Thread] = []
    if process.stdout is not None:
        threads.append(
            threading.Thread(
                target=_reader,
                args=(process.stdout, stdout_chunks, "info", "stdout"),
                daemon=True,
            )
        )
    if process.stderr is not None:
        threads.append(
            threading.Thread(
                target=_reader,
                args=(process.stderr, stderr_chunks, "warning", "stderr"),
                kwargs={"dynamic_level": True},
                daemon=True,
            )
        )

    for thread in threads:
        thread.start()

    exit_code = process.wait()
    for thread in threads:
        thread.join()

    result = {
        "command": cmd,
        "exit_code": exit_code,
        "stdout": "".join(stdout_chunks),
        "stderr": "".join(stderr_chunks),
    }
    if exit_code == 0:
        _log(
            logger,
            "info",
            f"Command finished with exit code {exit_code}.",
        )
    else:
        _log(
            logger,
            "warning",
            f"Command finished with exit code {exit_code}.",
        )
    return result


_COM_NAME_RE = re.compile(r"\bCOM\d+\b", re.IGNORECASE)


def _normalize_port(port: Any) -> str:
    """Normalize COM port name to upper case without surrounding whitespace."""
    return str(port).strip().upper()


def _parse_scan_ports(text: str) -> List[str]:
    """
    Extract COM port names from scan stdout/stderr.

    The tool's exact output format is not strictly documented, so we accept any
    occurrence of ``COMxx`` (case-insensitive) and return a de-duplicated list
    in first-seen order, normalized to upper case.
    """
    if not text:
        return []
    seen: Dict[str, None] = {}
    for match in _COM_NAME_RE.findall(text):
        seen.setdefault(match.upper(), None)
    return list(seen.keys())


def _normalize_baud(baud: Optional[Any]) -> str:
    """Convert input baud to command line baudrate string."""
    if baud is None:
        return DEFAULT_BAUDRATE

    baud_str = str(baud).strip()
    if not baud_str:
        return DEFAULT_BAUDRATE

    try:
        baud_int = int(baud_str, 10)
    except (TypeError, ValueError) as exc:
        raise ValueError(f"Invalid baud value: {baud}") from exc

    if baud_int <= 0:
        raise ValueError(f"Baud must be positive, got: {baud}")

    return str(baud_int)


def _resolve_cmd_tool() -> Path:
    """Resolve GD32MassProductionTool_CMD.exe path from script location."""
    return TOOLS_DIR / TOOL_PACKAGE_DIRNAME / "GD32MassProductionTool_CMD.exe"


def _resolve_tool_archive() -> Path:
    """Resolve GD32 mass production tool archive path."""
    return TOOLS_DIR / TOOL_ARCHIVE_NAME


def _sha256_file(file_path: Path) -> str:
    """Calculate SHA256 for one file."""
    with file_path.open("rb") as file_obj:
        if sys.version_info >= (3, 11):
            return hashlib.file_digest(file_obj, "sha256").hexdigest().upper()
        digest = hashlib.sha256()
        while True:
            chunk = file_obj.read(1024 * 1024)
            if not chunk:
                break
            digest.update(chunk)
        return digest.hexdigest().upper()


def _verify_cmd_hash(cmd_tool: Path) -> bool:
    """Verify command tool hash."""
    actual_hash = _sha256_file(cmd_tool)
    return actual_hash == EXPECTED_CMD_SHA256


def _extract_archive(archive_path: Path, output_parent: Path, logger: Optional[logging.Logger]) -> None:
    """Extract archive via Python stdlib."""
    _log(logger, "debug", f"Extract archive: {archive_path}")
    try:
        shutil.unpack_archive(str(archive_path), extract_dir=str(output_parent))
    except (shutil.ReadError, ValueError, OSError) as exc:
        raise RuntimeError(f"Failed to extract archive by stdlib: {exc}") from exc


def _ensure_cmd_tool_ready(logger: Optional[logging.Logger]) -> Path:
    """
    Ensure command tool exists and passes hash verification.

    If hash check fails, delete extracted directory and re-extract once.
    If still fails, raise exception to stop the flow.
    """
    cmd_tool = _resolve_cmd_tool()
    tool_dir = cmd_tool.parent
    archive_path = _resolve_tool_archive()
    output_parent = tool_dir.parent

    if cmd_tool.is_file() and _verify_cmd_hash(cmd_tool):
        _log(logger, "debug", "Tool hash verification succeeded.")
        return cmd_tool

    if not archive_path.is_file():
        raise FileNotFoundError(
            f"Tool archive not found for extract/repair: {archive_path}"
        )

    if not cmd_tool.is_file():
        _log(logger, "warning", f"Tool not found, extracting archive: {archive_path}")
        _extract_archive(archive_path, output_parent, logger)
    else:
        _log(logger, "warning", "Tool hash verification failed. Re-extracting tool package.")
        shutil.rmtree(tool_dir, ignore_errors=True)
        _extract_archive(archive_path, output_parent, logger)

    if not cmd_tool.is_file():
        raise FileNotFoundError(f"Tool missing after extract: {cmd_tool}")

    if _verify_cmd_hash(cmd_tool):
        _log(logger, "debug", "Tool hash verification succeeded after extract.")
        return cmd_tool

    _log(logger, "warning", "Tool hash verification failed. Re-extracting one more time.")
    if tool_dir.exists():
        shutil.rmtree(tool_dir, ignore_errors=True)

    _extract_archive(archive_path, output_parent, logger)
    if not cmd_tool.is_file():
        raise FileNotFoundError(f"Tool missing after re-extract: {cmd_tool}")

    if not _verify_cmd_hash(cmd_tool):
        raise RuntimeError("Tool hash verification failed after re-extract.")

    _log(logger, "debug", "Tool hash verification succeeded after re-extract.")
    return cmd_tool


def flash_firmware(
    device: Optional[str],
    port: Optional[str],
    baud: Optional[Any],
    start: Optional[str],  # Reserved for future use.
    binfile: Optional[str],
    logger: Optional[logging.Logger] = None,
    com_select_mode: Optional[Any] = None,
) -> Dict[str, Any]:
    """
    Flash firmware by GD32MassProductionTool_CMD.exe.

    Args:
        device: Board type, only GD32VW553H is supported currently.
        port: COM port id (e.g. COM3), required.
        baud: Download baudrate. Default to 2000000 when None/empty.
        start: Flash start address (currently not used by tool command).
        binfile: Firmware binary file path, required.
        logger: Optional logger object.
        com_select_mode: ``ignore``: scan then ignore other COMs;
          ``specify`` (default): scan then ``specify <port>``. When omitted, uses
          ``DEFAULT_COM_SELECT_MODE``.

    Returns:
        A structured dict containing flashing result and command outputs.

        - ``scan``: subprocess result for the ``scan`` step, or None if not run
        - ``ignore``: subprocess result for ``ignore <other_coms ...>``,
          None if skipped (no other COM detected) or not reached
        - ``download``: None (no tool steps ran) or a dict with:
            - ``specify``: subprocess result for ``specify <port>``
            - ``download``: subprocess result for the ``download ...`` step,
              or None if that step was not reached
        - ``cleanup``: dict with ``ignore`` or ``specify`` subprocess result
          from finally reset (``ignore nothing`` vs ``specify nothing`` per mode)

    Note:
        Uses ``os.chdir`` to the tool directory during the call. Not safe for
        concurrent ``flash_firmware`` invocations in the same process.
    """
    result: Dict[str, Any] = {
        "success": False,
        "message": "",
        "stage": "init",
        "device": device,
        "port": port,
        "baud": baud,
        "start": start,
        "binfile": binfile,
        "scan": None,
        "ignore": None,
        "download": None,
        "cleanup": None,
        "com_select_mode": None,
    }

    cmd_tool = _resolve_cmd_tool()
    cmd_cwd = cmd_tool.parent
    saved_cwd: Optional[str] = None

    try:
        if not device or not str(device).strip():
            raise ValueError("Parameter 'device' cannot be empty.")
        if not port or not str(port).strip():
            raise ValueError("Parameter 'port' cannot be empty.")
        if not binfile or not str(binfile).strip():
            raise ValueError("Parameter 'binfile' cannot be empty.")

        mode = _normalize_com_select_mode(com_select_mode)
        result["com_select_mode"] = mode

        chip = SUPPORTED_DEVICE_TO_CHIP.get(str(device).strip())
        if chip is None:
            raise ValueError(
                f"Unsupported device '{device}'. Only GD32VW553H is supported."
            )

        baudrate = _normalize_baud(baud)

        image_path = Path(str(binfile)).expanduser().resolve()
        if not image_path.is_file():
            raise FileNotFoundError(f"Firmware bin file not found: {image_path}")

        cmd_tool = _ensure_cmd_tool_ready(logger)
        cmd_cwd = cmd_tool.parent

        saved_cwd = os.getcwd()
        os.chdir(cmd_cwd)

        _log(
            logger,
            "debug",
            (
                f"Flashing parameters: device={device}, chip={chip}, port={port}, "
                f"baudrate={baudrate}, start={start}, image={image_path}, "
                f"with_jtag={DEFAULT_WITH_JTAG}, erase_option={DEFAULT_ERASE_OPTION}, "
                f"com_select_mode={mode}"
            ),
        )
        if start is not None:
            _log(
                logger,
                "warning",
                "Parameter 'start' is currently not used by GD32 download command.",
            )

        target_port = _normalize_port(port)

        result["stage"] = "scan"
        scan_cmd = [str(cmd_tool), "scan"]
        scan_ret = _run_cmd(scan_cmd, cmd_cwd, logger)
        result["scan"] = scan_ret

        if scan_ret["exit_code"] != 0:
            result["message"] = (
                f"Scan failed with exit code {scan_ret['exit_code']}."
            )
            return result

        scan_text = (scan_ret.get("stdout") or "") + "\n" + (scan_ret.get("stderr") or "")
        detected_ports = _parse_scan_ports(scan_text)
        _log(
            logger,
            "debug",
            f"Scan detected COM ports: {detected_ports if detected_ports else '<none>'}",
        )

        if target_port not in detected_ports:
            result["message"] = (
                f"Target port {target_port} not detected by scan. "
                f"Detected: {detected_ports if detected_ports else '<none>'}"
            )
            return result

        other_ports = [com for com in detected_ports if com != target_port]
        if mode == COM_SELECT_IGNORE:
            if other_ports:
                result["stage"] = "ignore"
                ignore_cmd = [str(cmd_tool), "ignore", *other_ports]
                ignore_ret = _run_cmd(ignore_cmd, cmd_cwd, logger)
                result["ignore"] = ignore_ret
                if ignore_ret["exit_code"] != 0:
                    err_excerpt = _result_error_excerpt(ignore_ret)
                    result["message"] = (
                        f"Failed to ignore other COM ports: {other_ports}. "
                        f"exit_code={ignore_ret['exit_code']}. "
                        f"detail={err_excerpt or '<empty>'}"
                    )
                    return result
            else:
                _log(
                    logger,
                    "debug",
                    "No other COM ports detected; skip ignore step.",
                )
        else:
            _log(
                logger,
                "debug",
                "COM selection mode is specify; skipping ignore step.",
            )

        download_steps: Dict[str, Any] = {"specify": None, "download": None}
        result["download"] = download_steps

        if mode == COM_SELECT_SPECIFY:
            result["stage"] = "specify"
            specify_cmd = [str(cmd_tool), "specify", target_port]
            specify_ret = _run_cmd(specify_cmd, cmd_cwd, logger)
            download_steps["specify"] = specify_ret
            if specify_ret["exit_code"] != 0:
                err_excerpt = _result_error_excerpt(specify_ret)
                result["message"] = (
                    f"Failed to specify target port {target_port}. "
                    f"exit_code={specify_ret['exit_code']}. "
                    f"detail={err_excerpt or '<empty>'}"
                )
                return result

        result["stage"] = "download"
        download_cmd = [
            str(cmd_tool),
            "download",
            chip,
            baudrate,
            DEFAULT_WITH_JTAG,
            str(image_path),
            DEFAULT_ERASE_OPTION,
        ]
        download_ret = _run_cmd(download_cmd, cmd_cwd, logger)
        download_steps["download"] = download_ret

        if download_ret["exit_code"] == 0:
            result["success"] = True
            result["message"] = "Download succeeded."
        else:
            err_excerpt = _result_error_excerpt(download_ret)
            result["message"] = (
                f"Download failed. exit_code={download_ret['exit_code']}. "
                f"detail={err_excerpt or '<empty>'}"
            )

        return result

    except Exception as exc:  # Unknown and known errors are handled uniformly.
        result["stage"] = "exception"
        result["message"] = f"Unexpected error: {exc}"
        result["error_type"] = type(exc).__name__
        result["traceback"] = traceback.format_exc()
        _log(logger, "error", result["message"])
        return result

    finally:
        cleanup_steps: Dict[str, Any] = {"ignore": None, "specify": None}
        result["cleanup"] = cleanup_steps

        mode_tag = result.get("com_select_mode")
        if mode_tag == COM_SELECT_IGNORE:
            cleanup_plan = (
                ("ignore", [str(cmd_tool), "ignore", "nothing"]),
            )
        elif mode_tag == COM_SELECT_SPECIFY:
            cleanup_plan = (
                ("specify", [str(cmd_tool), "specify", "nothing"]),
            )
        else:
            # Mode unknown (e.g. failed before assignment): reset both for safety.
            cleanup_plan = (
                ("ignore", [str(cmd_tool), "ignore", "nothing"]),
                ("specify", [str(cmd_tool), "specify", "nothing"]),
            )
        for step_name, step_cmd in cleanup_plan:
            try:
                if cmd_tool.is_file():
                    cleanup_steps[step_name] = _run_cmd(step_cmd, cmd_cwd, logger)
                else:
                    cleanup_steps[step_name] = {
                        "command": step_cmd,
                        "exit_code": None,
                        "stdout": "",
                        "stderr": "Cleanup skipped because tool file does not exist.",
                    }
            except Exception as cleanup_exc:
                cleanup_steps[step_name] = {
                    "command": step_cmd,
                    "exit_code": None,
                    "stdout": "",
                    "stderr": f"Cleanup exception: {cleanup_exc}",
                }
                _log(
                    logger,
                    "error",
                    f"Cleanup '{step_name} nothing' failed: {cleanup_exc}",
                )

        if saved_cwd is not None:
            try:
                os.chdir(saved_cwd)
            except OSError as ose:
                _log(
                    logger,
                    "warning",
                    f"Failed to restore working directory to {saved_cwd!r}: {ose}",
                )


def _build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Flash firmware with GD32MassProductionTool_CMD.exe",
    )
    parser.add_argument("--device", required=True, help="Board device, e.g. GD32VW553H")
    parser.add_argument("--port", required=True, help="COM port, e.g. COM3")
    parser.add_argument(
        "--baud",
        default=None,
        help=f"Baudrate for download, default {DEFAULT_BAUDRATE} when omitted/empty",
    )
    parser.add_argument(
        "--start",
        default=None,
        help="Start address (reserved, currently not used by tool command)",
    )
    parser.add_argument("--binfile", required=True, help="Absolute/relative bin file path")
    return parser


def main() -> int:
    args = _build_arg_parser().parse_args()
    result = flash_firmware(
        device=args.device,
        port=args.port,
        baud=args.baud,
        start=args.start,
        binfile=args.binfile,
        logger=None,
    )
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 0 if result.get("success") else 1


if __name__ == "__main__":
    raise SystemExit(main())
