#!/usr/bin/env python3
# coding=utf-8

import os
import platform
import shutil
import requests
import hashlib
import tarfile
import zipfile

COUNTRY_CODE = os.environ.get("OPEN_COUNTRY_CODE", "")


def set_country_code():
    global COUNTRY_CODE
    if len(COUNTRY_CODE):
        return COUNTRY_CODE

    try:
        response = requests.get("http://www.ip-api.com/json", timeout=5)
        response.raise_for_status()

        result = response.json()
        country = result.get("country", "")
        print(f"country code: {country}")

        COUNTRY_CODE = country
    except requests.exceptions.RequestException as e:
        print(f"country code error: {e}, default to China mirror")
        COUNTRY_CODE = "China"

    return COUNTRY_CODE


def get_country_code():
    global COUNTRY_CODE
    if len(COUNTRY_CODE):
        return COUNTRY_CODE
    return set_country_code()


SYSTEM_NAME = ""


def set_system_name():
    global SYSTEM_NAME
    _env = platform.system().lower()
    if "linux" in _env:
        SYSTEM_NAME = "linux"
    elif "darwin" in _env:
        machine = "x86" if "x86" in platform.machine().lower() else "arm64"
        SYSTEM_NAME = f"darwin_{machine}"
    else:
        SYSTEM_NAME = "windows"
    return SYSTEM_NAME


def get_system_name():
    global SYSTEM_NAME
    if len(SYSTEM_NAME):
        return SYSTEM_NAME
    return set_system_name()


def rm_rf(file_path):
    if os.path.isfile(file_path):
        os.remove(file_path)
    elif os.path.isdir(file_path):
        shutil.rmtree(file_path)
    return True


def copy_file(source, target, force=True) -> bool:
    if not os.path.exists(source):
        print(f"Not found [{source}].")
        return False
    if not force and os.path.exists(target):
        return True

    target_dir = os.path.dirname(target)
    if target_dir:
        os.makedirs(target_dir, exist_ok=True)
    shutil.copy(source, target)
    return True


def calc_sha256(file_path: str) -> str:
    sha256_hash = hashlib.sha256()

    try:
        with open(file_path, "rb") as f:
            for byte_block in iter(lambda: f.read(4096), b""):
                sha256_hash.update(byte_block)
        return sha256_hash.hexdigest()
    except FileNotFoundError:
        print(f"Error: Not found {file_path}")
        return ""
    except PermissionError:
        print(f"Error: Permission not support {file_path}.")
        return ""
    except Exception as e:
        print(f"Exception: {str(e)}")
        return ""


def extract_archive(file_path: str, dest_dir: str) -> bool:
    if not os.path.exists(file_path):
        print(f"Error: Not found {file_path}.")
        return False

    os.makedirs(dest_dir, exist_ok=True)

    try:
        if file_path.endswith(".zip"):
            with zipfile.ZipFile(file_path, "r") as zip_ref:
                zip_ref.extractall(path=dest_dir)

        elif file_path.endswith(".tar.gz"):
            with tarfile.open(file_path, "r:gz") as tar:
                tar.extractall(path=dest_dir)

        elif file_path.endswith(".tar.bz2"):
            with tarfile.open(file_path, "r:bz2") as tar:
                tar.extractall(path=dest_dir)
        elif file_path.endswith(".tar.xz"):
            with tarfile.open(file_path, "r:xz") as tar:
                tar.extractall(path=dest_dir)
        else:
            print(f"Error: extract not support {file_path}.")
            return False

    except zipfile.BadZipFile:
        print(f"Error: Invalid zip file {file_path}.")
        return False
    except tarfile.TarError:
        print(f"Error: Invalid tar file {file_path}.")
        return False
    except Exception as e:
        print(f"Exception: {str(e)}")
        return False

    return True
