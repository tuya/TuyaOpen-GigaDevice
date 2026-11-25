#!/usr/bin/env python3
# coding=utf-8
import importlib.util
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
from types import SimpleNamespace
from unittest.mock import MagicMock, patch


BRIDGE_PATH = Path(__file__).resolve().parents[1] / "platform_flash_bridge.py"


def _load_bridge():
    spec = importlib.util.spec_from_file_location(
        "gd32_platform_flash_bridge_under_test",
        BRIDGE_PATH,
    )
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class TestGd32FlashFallback(unittest.TestCase):
    def setUp(self):
        self.bridge = _load_bridge()
        self.primary = MagicMock()
        self.fallback = MagicMock()
        self.logger = MagicMock()

    def _flash(self, start=None, binfile="firmware.bin"):
        return self.bridge.flash_firmware(
            device="GD32VW553H",
            port="COM5",
            baud=2000000,
            start=start,
            binfile=binfile,
            logger=self.logger,
        )

    def test_scan_crash_falls_back_to_serial_isp(self):
        self.primary.flash_firmware.return_value = {
            "success": False,
            "stage": "scan",
            "fallback_allowed": True,
            "message": "Scan failed with exit code -1073741819.",
            "scan": {"exit_code": -1073741819},
            "download": None,
        }
        self.fallback.flash_firmware.return_value = {
            "success": True,
            "stage": "done",
            "message": "Download succeeded.",
        }

        with patch.object(self.bridge.os, "name", "nt"), \
             patch.object(self.bridge, "_load_flash_module",
                          return_value=self.primary), \
             patch.object(self.bridge, "_load_named_flash_module",
                          return_value=self.fallback) as load_fallback:
            result = self._flash(start="0x08010000")

        load_fallback.assert_called_once_with("gd32_isp_flash")
        self.assertEqual(self.primary.flash_firmware.call_args.kwargs["start"], "0x08010000")
        self.assertIsNone(self.fallback.flash_firmware.call_args.kwargs["start"])
        self.assertTrue(result["success"])
        self.assertEqual(result["fallback"]["from"], "gd32_mass_flash")
        self.assertEqual(result["fallback"]["to"], "gd32_isp_flash")
        self.logger.warning.assert_called_once()

    def test_port_not_detected_before_download_falls_back(self):
        self.primary.flash_firmware.return_value = {
            "success": False,
            "stage": "scan",
            "fallback_allowed": True,
            "message": "Target port COM5 not detected by scan.",
            "scan": {"exit_code": 0},
            "download": None,
        }
        self.fallback.flash_firmware.return_value = {
            "success": True,
            "stage": "done",
        }

        with patch.object(self.bridge.os, "name", "nt"), \
             patch.object(self.bridge, "_load_flash_module",
                          return_value=self.primary), \
             patch.object(self.bridge, "_load_named_flash_module",
                          return_value=self.fallback):
            result = self._flash()

        self.assertTrue(result["success"])

    def test_download_failure_does_not_retry(self):
        primary_result = {
            "success": False,
            "stage": "download",
            "fallback_allowed": False,
            "message": "Download failed.",
            "scan": {"exit_code": 0},
            "download": {
                "specify": {"exit_code": 0},
                "download": {"exit_code": 1},
            },
        }
        self.primary.flash_firmware.return_value = primary_result

        with patch.object(self.bridge.os, "name", "nt"), \
             patch.object(self.bridge, "_load_flash_module",
                          return_value=self.primary), \
             patch.object(self.bridge, "_load_named_flash_module",
                          return_value=self.fallback) as load_fallback:
            result = self._flash()

        load_fallback.assert_not_called()
        self.assertIs(result, primary_result)

    def test_download_exception_checks_whether_process_started(self):
        mass = self.bridge._load_named_flash_module("gd32_mass_flash")
        self.fallback.flash_firmware.return_value = {"success": True, "stage": "done"}

        for started in (False, True):
            with self.subTest(started=started), TemporaryDirectory() as tmpdir:
                tool = Path(tmpdir) / "tool.exe"
                firmware = Path(tmpdir) / "firmware.bin"
                tool.touch()
                firmware.touch()
                self.fallback.reset_mock()

                def popen(cmd, **kwargs):
                    process = MagicMock(stdout=None, stderr=None)
                    process.wait.return_value = 0
                    if cmd[1] == "scan":
                        process.stdout = MagicMock()
                        process.stdout.readline.side_effect = ["COM5\n", ""]
                    if cmd[1] == "download":
                        if not started:
                            raise FileNotFoundError("download process could not start")
                        process.wait.side_effect = OSError("failed waiting for download")
                    return process

                with (
                    patch.object(self.bridge, "os", SimpleNamespace(name="nt")),
                    patch.object(self.bridge, "_load_flash_module", return_value=mass),
                    patch.object(self.bridge, "_load_named_flash_module", return_value=self.fallback),
                    patch.object(mass, "_ensure_cmd_tool_ready", return_value=tool),
                    patch.object(mass.subprocess, "Popen", side_effect=popen),
                ):
                    result = self._flash(binfile=str(firmware))

                if started:
                    self.fallback.flash_firmware.assert_not_called()
                    self.assertFalse(result["success"])
                    self.assertEqual(result["stage"], "exception")
                    self.assertIn("failed waiting for download", result["message"])
                else:
                    self.fallback.flash_firmware.assert_called_once()
                    self.assertTrue(result["success"])


if __name__ == "__main__":
    unittest.main()
