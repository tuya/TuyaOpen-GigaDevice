#!/usr/bin/env python3
# coding=utf-8

import os
import sys
import platform

from tools.download_toolchain import download_toolchain


def main():
    if not platform.system().lower().startswith("windows"):
        print("Error: GD32 toolchain prepare only supports Windows.")
        sys.exit(1)

    root = os.path.dirname(os.path.abspath(__file__))
    platform_root = os.path.abspath(os.path.join(root, ".."))
    toolchain_root = os.path.join(platform_root, "tools")
    os.makedirs(toolchain_root, exist_ok=True)
    print(f"toolchain_root: {toolchain_root}")

    if not download_toolchain(toolchain_root):
        print("Error: download toolchain failed.")
        sys.exit(1)

    sys.exit(0)


if __name__ == "__main__":
    main()
