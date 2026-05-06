#!/usr/bin/env python3
# coding=utf-8
# 参数说明：
# $1 - params path: $1/build_param.[cmake/config/json]
# $2 - user cmd: build/clean/...

import os
import sys
import json
import subprocess
import shutil

def clean(root):
    shutil.rmtree("build", ignore_errors=True)
    pass

def parser_para_file(json_file):
    if not os.path.isfile(json_file):
        print(f"Error: Not found [{json_file}].")
        return {}
    try:
        with open(json_file, 'r', encoding='utf-8') as f:
            json_data = json.load(f)
    except Exception as e:
        print(f"Parser json error:  [{str(e)}].")
        return {}
    return json_data

def need_settarget(target_file, target):
    if not os.path.exists(target_file):
        return True
    with open(target_file, "r", encoding='utf-8') as f:
        old_target = f.read().strip()
    print(f"old_target: {old_target}")
    if target != old_target:
        return True
    return False

def record_target(target_file, target):
    with open(target_file, "w", encoding='utf-8') as f:
        f.write(target)
    return True

def need_settarget(target_file, target):
    if not os.path.exists(target_file):
        return True
    with open(target_file, "r", encoding='utf-8') as f:
        old_target = f.read().strip()
    print(f"old_target: {old_target}")
    if target != old_target:
        return True
    return False


def copy_file(source, target, force=True) -> bool:
    '''
    force: Overwrite if the target file exists
    '''
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

def _copy_debug_artifacts(build_root, app_name, debug_path):
    """将编译产生的 .map / .lst / .dump 拷贝到 debug 目录。"""
    elf_bin_dir = os.path.join(
        build_root, 'gd32_os', 'MSDK', 'projects', 'cmake', 'output', 'bin')

    for ext in ('map', 'lst', 'dump'):
        src = os.path.join(elf_bin_dir, f"{app_name}.{ext}")
        copy_file(src, os.path.join(debug_path, f"{app_name}.{ext}"))


def copy_bins(build_root, param_data):
    bin_output_path = os.path.join(build_root, 'gd32_os', 'scripts', 'images')
    app_all_bin = os.path.join(bin_output_path, "image-all.bin")
    if not os.path.exists(app_all_bin):
        print(f"Error: Not found {app_all_bin}.")
        return False

    ua_file_bin = os.path.join(bin_output_path, "image-ota.bin")
    ug_file_bin = os.path.join(bin_output_path, "image-ota.bin")

    app_name = param_data["CONFIG_PROJECT_NAME"]
    app_ver = param_data["CONFIG_PROJECT_VERSION"]
    output_path = param_data["BIN_OUTPUT_DIR"]
    debug_path = os.path.join(output_path, "debug")
    os.makedirs(output_path, exist_ok=True)
    os.makedirs(debug_path, exist_ok=True)
    try:
        copy_file(app_all_bin,
                  os.path.join(output_path, f"{app_name}_QIO_{app_ver}.bin"))
        copy_file(ua_file_bin,
                  os.path.join(output_path, f"{app_name}_UA_{app_ver}.bin"))
        copy_file(ug_file_bin,
                  os.path.join(output_path, f"{app_name}_UG_{app_ver}.bin"))
        _copy_debug_artifacts(build_root, app_name, debug_path)
    except Exception as e:
        print(f"Error: copy assets: {str(e)}")
        return False

def main():
    if len(sys.argv) < 2:
        print(f"Error: At least 2 parameters are needed {sys.argv}.")
    build_param_path = sys.argv[1]
    user_cmd = sys.argv[2]

    build_param_file = os.path.join(build_param_path, "build_param.json")
    param_data = parser_para_file(build_param_file)
    if not len(param_data):
        sys.exit(1)

    root = os.path.dirname(os.path.abspath(__file__))
    build_root = os.path.join(root, "gd32_os")
    app_target_file = os.path.join(build_root, ".app")
    app_name = param_data["CONFIG_PROJECT_NAME"]

    root = os.path.dirname(os.path.abspath(__file__))
    if "clean" == user_cmd or need_settarget(app_target_file, app_name):
        clean(root)
        if "clean" == user_cmd:
            sys.exit(0)

    record_target(app_target_file, app_name)

    if not os.path.isfile("CMakeLists.txt"):
        print("Error: CMakeLists.txt not found in the current directory.")
        sys.exit(1)

    try:
        os.makedirs("build", exist_ok=True)

        # Prefer Ninja for faster incremental builds; use updated toolchain file
        toolchain_path = os.path.join(os.getcwd(), "toolchain_file.cmake")
        cmake_command = [
            "cmake", "-S", ".", "-B", "build", "-G", "Ninja",
            f"-DBUILD_PARAM_PATH={build_param_path}", f"-DCMAKE_TOOLCHAIN_FILE={toolchain_path}"
        ]

        subprocess.run(
            cmake_command,
            check=True,
        )
        subprocess.run(["cmake", "--build", "build"], check=True)
    except subprocess.CalledProcessError as e:
        print(f"CMake process failed: {e}")
        sys.exit(1)

    copy_bins(root, param_data)

    sys.exit(0)

if __name__ == "__main__":
    main()
