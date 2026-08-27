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

# The SDK links prebuilt vendor libraries (libwifi.a, libwpas.a, libble_max.a)
# built with GCC 10.2, and the build selects the rv32imafcbp multilib - b and p
# are Nuclei draft extensions that mainline RISC-V GCC does not implement. Only
# Nuclei's own 2022.04 toolchain works here; a distro riscv64-unknown-elf-gcc
# rejects -march=rv32imafcbp outright.
#
# Windows: Tuya rehosts the package, repacked with a <folder>/ top level so it
#   unpacks straight into platform/tools/.
# Linux:   Nuclei's official tarball, whose top level is gcc/ - it has to be
#   unpacked into a directory we name ourselves (extract_into_folder). 2022.04
#   is no longer listed on the vendor download page and is only reachable by
#   direct URL, so mirroring it next to the Windows package is worth doing;
#   until then both CN and overseas point at the vendor host.
TOOLCHAIN_PACKAGES = {
    "windows": {
        "name": "nuclei_riscv_newlibc_prebuilt_win32_2022.04.zip",
        "folder": "nuclei_riscv_newlibc_prebuilt_win32_2022.04",
        "size": 282608264,
        "sha256": (
            "af82b229742e893266409b82da7d2f22ef2b40bd4a9adbe37eaa82af7300b165"
        ),
        "url_cn": (
            "https://images.tuyacn.com/rms-static/"
            "c91a8df0-5b09-11f1-8d53-258e63d3fe0e-1780023301199.zip"
            "?tyName=nuclei_riscv_newlibc_prebuilt_win32_2022.04.zip"
        ),
        "url_overseas": (
            "https://github.com/tuya/TuyaOpen-GigaDevice/releases/download/"
            "tools/nuclei_riscv_newlibc_prebuilt_win32_2022.04.zip"
        ),
        "gcc_exe": "riscv-nuclei-elf-gcc.exe",
        "extract_into_folder": False,
        "machines": ("amd64", "x86_64"),
    },
    "linux": {
        "name": "nuclei_riscv_newlibc_prebuilt_linux64_2022.04.tar.bz2",
        "folder": "nuclei_riscv_newlibc_prebuilt_linux64_2022.04",
        "size": 223321817,
        "sha256": (
            "cab6f57d58f3ca6931e9204eb582217a7aa4e02004eaa55cb06ec8ef4e529981"
        ),
        "url_cn": (
            "https://download.nucleisys.com/upload/files/toolchain/gcc/"
            "nuclei_riscv_newlibc_prebuilt_linux64_2022.04.tar.bz2"
        ),
        "url_overseas": (
            "https://download.nucleisys.com/upload/files/toolchain/gcc/"
            "nuclei_riscv_newlibc_prebuilt_linux64_2022.04.tar.bz2"
        ),
        "gcc_exe": "riscv-nuclei-elf-gcc",
        "extract_into_folder": True,
        "machines": ("x86_64", "amd64"),
    },
}

LAST_PROGRESS = 0
MAX_DOWNLOAD_ATTEMPTS = 2


def get_toolchain_package_info():
    country_code = get_country_code()
    system = platform.system().lower()
    machine = platform.machine().lower()
    print(f"get package from [{country_code} {system}_{machine}]")

    package = TOOLCHAIN_PACKAGES.get(system)
    if package is None:
        print("##############################")
        print(f"Warning: GD32 toolchain not support [{system}_{machine}]")
        print(f"Supported: {', '.join(sorted(TOOLCHAIN_PACKAGES))}")
        print("##############################")
        return {}

    if machine not in package["machines"]:
        print("##############################")
        print(f"Warning: GD32 toolchain not support [{system}_{machine}]")
        print(f"Only 64-bit x86 is prebuilt for {system}.")
        print("##############################")
        return {}

    if country_code == "China":
        url = package["url_cn"]
    else:
        url = package["url_overseas"]

    package_info = {
        "url": url,
        "name": package["name"],
        "size": package["size"],
        "sha256": package["sha256"],
        "folder": package["folder"],
        "gcc_exe": package["gcc_exe"],
        "extract_into_folder": package["extract_into_folder"],
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
    # The Windows zip carries its own <folder>/ top level and unpacks straight
    # into platform/tools/. Nuclei's Linux tarball starts at gcc/, so unpacking
    # it the same way would leave the toolchain in platform/tools/gcc.
    dest = toolchain_root
    if package_info.get("extract_into_folder"):
        dest = os.path.join(toolchain_root, package_info["folder"])
    print(f"[Extracting toolchain package]: {package} -> {dest}")
    return extract_archive(package, dest)


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


def _toolchain_gcc_exists(folder_path, package_info=None) -> bool:
    gcc_exe = (package_info or {}).get("gcc_exe", "riscv-nuclei-elf-gcc")
    gcc_path = os.path.join(folder_path, "gcc", "bin", gcc_exe)
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
        if _toolchain_gcc_exists(folder_path, package_info):
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
            if _toolchain_gcc_exists(folder_path, package_info):
                return True
            print(f"Error: toolchain gcc not found under {folder_path}")

        cleanup_toolchain_cache(toolchain_root, package_info, remove_folder=True)

    return False
