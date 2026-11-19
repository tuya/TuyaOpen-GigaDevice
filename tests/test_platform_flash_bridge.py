#!/usr/bin/env python3
# coding=utf-8
import importlib.util
import unittest
from pathlib import Path
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

    def _flash(self):
        return self.bridge.flash_firmware(
            device="GD32VW553H",
            port="COM5",
            baud=2000000,
            start=None,
            binfile="firmware.bin",
            logger=self.logger,
        )

    def test_scan_crash_falls_back_to_serial_isp(self):
        self.primary.flash_firmware.return_value = {
            "success": False,
            "stage": "scan",
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
            result = self._flash()

        load_fallback.assert_called_once_with("gd32_isp_flash")
        self.assertTrue(result["success"])
        self.assertEqual(result["fallback"]["from"], "gd32_mass_flash")
        self.assertEqual(result["fallback"]["to"], "gd32_isp_flash")
        self.logger.warning.assert_called_once()

    def test_port_not_detected_before_download_falls_back(self):
        self.primary.flash_firmware.return_value = {
            "success": False,
            "stage": "scan",
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


if __name__ == "__main__":
    unittest.main()
