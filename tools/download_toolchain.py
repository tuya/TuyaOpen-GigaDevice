#!/usr/bin/env python3
# coding=utf-8

import os
import platform
import urllib.request
import ssl

from tools.util import (
    get_country_code,
    rm_rf,
    calc_sha256,
    extract_archive,
)

TOOLCHAIN_NAME = "nuclei_riscv_newlibc_prebuilt_win32_2022.04.zip"
TOOLCHAIN_FOLDER = "nuclei_riscv_newlibc_prebuilt_win32_2022.04"
TOOLCHAIN_SIZE = 282608264
TOOLCHAIN_SHA256 = (
    "af82b229742e893266409b82da7d2f22ef2b40bd4a9adbe37eaa82af7300b165"
)

URL_CN = (
    "https://images.tuyacn.com/rms-static/c91a8df0-5b09-11f1-8d53-258e63d3fe0e"
    "-1780023301199.zip?tyName=nuclei_riscv_newlibc_prebuilt_win32_2022.04.zip"
)
URL_OVERSEAS = (
    "https://github.com/tuya/TuyaOpen-GigaDevice/releases/download/tools/"
    "nuclei_riscv_newlibc_prebuilt_win32_2022.04.zip"
)

LAST_PROGRESS = 0
MAX_DOWNLOAD_ATTEMPTS = 2


def get_toolchain_package_info():
    country_code = get_country_code()
    system = platform.system().lower()
    sys_mac = f"{system}_{platform.machine().lower()}"
    print(f"get package from [{country_code} {sys_mac}]")

    if not sys_mac.startswith("windows"):
        print("##############################")
        print(f"Warning: GD32 toolchain not support [{sys_mac}]")
        print("Only Windows is supported currently.")
        print("##############################")
        return {}

    if country_code == "China":
        url = URL_CN
    else:
        url = URL_OVERSEAS

    package_info = {
        "url": url,
        "name": TOOLCHAIN_NAME,
        "size": TOOLCHAIN_SIZE,
        "sha256": TOOLCHAIN_SHA256,
        "folder": TOOLCHAIN_FOLDER,
    }
    return package_info


def _show_progress(block_num, block_size, total_size):
    global LAST_PROGRESS
    downloaded = block_num * block_size
    progress = 0
    if total_size > 0:
        progress = min(100, (downloaded / total_size) * 100)

    progress = int(progress)
    if LAST_PROGRESS != progress:
        print(f"\rprogress: {progress}%", end="")
        LAST_PROGRESS = progress if progress < 100 else 0


def _download_from_url(url, download_file):
    try:
        context = ssl._create_unverified_context()
        with urllib.request.urlopen(url, context=context) as response:
            total_size = int(response.headers.get("Content-Length", 0))
            block_size = 8192
            bytes_downloaded = 0

            with open(download_file, "wb") as f:
                while True:
                    buffer = response.read(block_size)
                    if not buffer:
                        break

                    f.write(buffer)
                    bytes_downloaded += len(buffer)
                    block_num = bytes_downloaded // block_size

                    _show_progress(block_num, block_size, total_size)

        print("")
    except Exception as e:
        print(f"Error: download failed: {str(e)}")
        rm_rf(download_file)
        return False

    return True


def wget_toolchain_package(toolchain_root, package_info) -> bool:
    url = package_info["url"]
    name = package_info["name"]
    download_file = os.path.join(toolchain_root, name)

    if os.path.exists(download_file):
        print(f"[Toolchain package is exiets]: {download_file}")
        return True

    print(f"[Downloading package]: {url}")
    if not _download_from_url(url, download_file):
        return False

    return True


def check_toolchain_package(toolchain_root, package_info) -> bool:
    name = package_info["name"]
    package = os.path.join(toolchain_root, name)

    real_size = os.stat(package).st_size
    exp_size = package_info["size"]
    if real_size != exp_size:
        print(f"Error size: real[{real_size}], expectation[{exp_size}]")
        return False

    real_sha256 = calc_sha256(package)
    exp_sha256 = package_info["sha256"]
    if real_sha256 != exp_sha256:
        print(f"Error sha256: real[{real_sha256}], expectation[{exp_sha256}]")
        return False

    print("Toolchain package is OK.")
    return True


def unzip_toolchain_package(toolchain_root, package_info) -> bool:
    name = package_info["name"]
    package = os.path.join(toolchain_root, name)
    print(f"[Extracting toolchain package]: {package}")
    return extract_archive(package, toolchain_root)


def cleanup_toolchain_cache(toolchain_root, package_info, remove_folder=False) -> bool:
    name = package_info["name"]
    package = os.path.join(toolchain_root, name)
    if os.path.exists(package):
        print(f"[Removing invalid toolchain package]: {package}")
        rm_rf(package)

    if remove_folder:
        folder = package_info["folder"]
        folder_path = os.path.join(toolchain_root, folder)
        if os.path.exists(folder_path):
            print(f"[Removing incomplete toolchain folder]: {folder_path}")
            rm_rf(folder_path)

    return True


def prepare_toolchain_package(toolchain_root, package_info) -> bool:
    name = package_info["name"]
    package = os.path.join(toolchain_root, name)

    if os.path.exists(package):
        if check_toolchain_package(toolchain_root, package_info):
            return True
        cleanup_toolchain_cache(toolchain_root, package_info)

    if not wget_toolchain_package(toolchain_root, package_info):
        cleanup_toolchain_cache(toolchain_root, package_info)
        return False

    if not check_toolchain_package(toolchain_root, package_info):
        cleanup_toolchain_cache(toolchain_root, package_info)
        return False

    return True


def _toolchain_gcc_exists(folder_path) -> bool:
    gcc_path = os.path.join(folder_path, "gcc", "bin", "riscv-nuclei-elf-gcc.exe")
    return os.path.isfile(gcc_path)


def download_toolchain(toolchain_root) -> bool:
    package_info = get_toolchain_package_info()
    if not package_info:
        return False

    name = package_info["name"]
    folder = package_info["folder"]
    package_path = os.path.join(toolchain_root, name)
    folder_path = os.path.join(toolchain_root, folder)
    if os.path.exists(folder_path):
        if _toolchain_gcc_exists(folder_path):
            if os.path.exists(package_path) and not check_toolchain_package(
                toolchain_root, package_info
            ):
                cleanup_toolchain_cache(
                    toolchain_root, package_info, remove_folder=True
                )
            else:
                print(f"[Toolchain folder is exists]: {folder_path}")
                return True
        else:
            print(f"[Incomplete toolchain folder]: {folder_path}")
            cleanup_toolchain_cache(toolchain_root, package_info, remove_folder=True)

    for idx in range(MAX_DOWNLOAD_ATTEMPTS):
        if idx > 0:
            print(
                f"[Retrying toolchain download]: "
                f"{idx + 1}/{MAX_DOWNLOAD_ATTEMPTS}"
            )

        if not prepare_toolchain_package(toolchain_root, package_info):
            continue

        rm_rf(folder_path)
        if unzip_toolchain_package(toolchain_root, package_info):
            if _toolchain_gcc_exists(folder_path):
                return True
            print(f"Error: toolchain gcc not found under {folder_path}")

        cleanup_toolchain_cache(toolchain_root, package_info, remove_folder=True)

    return False
