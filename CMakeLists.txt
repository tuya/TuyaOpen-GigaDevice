cmake_minimum_required(VERSION 3.15)

if(NOT DEFINED BUILD_PARAM_PATH)
    message(FATAL_ERROR "BUILD_PARAM_PATH is not defined, should point to params directory")
endif()

if(EXISTS ${BUILD_PARAM_PATH}/build_param.cmake)
    include(${BUILD_PARAM_PATH}/build_param.cmake)
else()
    message(FATAL_ERROR "${BUILD_PARAM_PATH}/build_param.cmake not found")
endif()

# Provide PLATFORM_PATH expected by merged_toolchain (assume this CMakeLists sits in platform/GD32)
set(PLATFORM_PATH ${CMAKE_CURRENT_LIST_DIR})

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    # Auto use merged toolchain unless user specified another
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/merged_toolchain.cmake CACHE FILEPATH "Merged GD32 toolchain file" FORCE)
endif()

include(${CMAKE_TOOLCHAIN_FILE})
toolchain_reload_compiler()
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

project(TuyaOpen LANGUAGES C CXX ASM)


# Set a default build type if none was specified, and print it
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "Choose the type of build: Debug, Release, RelWithDebInfo, MinSizeRel" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Set global compiler flags
add_compile_options(-fdiagnostics-color=always)
option(ENABLE_TUYA_TRIM "Enable adapter fine-grained trimming and section GC" ON)
option(ENABLE_ADAPTER_WHOLE_ARCHIVE "Force whole-archive for all adapter sub-libs when trimmed" OFF)
option(ENABLE_STRICT_BLE_VENDOR_ISOLATION "Force using vendor BLE exported headers; demote or remove NimBLE include paths" ON)

if(ENABLE_TUYA_TRIM)
    add_compile_options(-ffunction-sections -fdata-sections)
    # 记录：链接阶段在最终目标附加 --gc-sections
endif()

# # Add an option to enable AddressSanitizer (ASan)
# option(ENABLE_ASAN "Enable AddressSanitizer" OFF)

# if(ENABLE_ASAN)
#     if(CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE MATCHES "Debug")
#         message(WARNING "ASan is best used with Debug build type. Current build type: ${CMAKE_BUILD_TYPE}")
#     endif()
#     message(STATUS "AddressSanitizer enabled")
#     add_compile_options(-fsanitize=address)
# endif()

set(TUYA_ADAPTER_PATH "tuyaos/tuyaos_adapter")
add_subdirectory(${TUYA_ADAPTER_PATH})

# 真实入口 main.c 由 gd32_os MSDK/app/CMakeLists.txt 自行添加，不在顶层再添加。
# 为了让生成的最终 elf 名称与 CONFIG_PROJECT_NAME 一致，覆盖 gd32_os 内部使用的 TARGET_EXE。
set(TARGET_EXE ${CONFIG_PROJECT_NAME} CACHE STRING "Override gd32_os main target name")

set(_OPEN_HEADER_DIRS)
if(OPEN_HEADER_DIR)
    string(REPLACE ";" " " HEADER_DIR_LIST "${OPEN_HEADER_DIR}")
    separate_arguments(HEADER_DIR_LIST)
    foreach(header_dir ${HEADER_DIR_LIST})
        list(APPEND _OPEN_HEADER_DIRS ${header_dir})
    endforeach()
endif()

# ---------------------------------------------------------------------------
# Integrate gd32_os subtree (provides RTOS, peripheral libs etc.)
# ---------------------------------------------------------------------------
set(GD32_OS_ROOT ${CMAKE_CURRENT_LIST_DIR}/gd32_os)
if(EXISTS ${GD32_OS_ROOT}/CMakeLists.txt)
    # Pass OS selection variables if present from build_param.cmake
    if(NOT DEFINED CONFIG_OS)
        set(CONFIG_OS FREERTOS)
    endif()
    add_subdirectory(${GD32_OS_ROOT})
else()
    message(WARNING "gd32_os not found at ${GD32_OS_ROOT}, skipping OS integration")
endif()

if(TARGET ${TARGET_EXE})
    # 分层添加必要的公共头路径，避免全局污染；优先顺序：项目生成 -> 平台适配 -> gd32_os 导出
    if(ENABLE_STRICT_BLE_VENDOR_ISOLATION)
        # 使用 BEFORE 强制 vendor BLE 头在所有搜索路径前
        target_include_directories(${TARGET_EXE} BEFORE
            PUBLIC
                ${PLATFORM_PATH}/gd32_os/MSDK/blesw/src/export
        )
    endif()
    target_include_directories(${TARGET_EXE}
        PRIVATE
            ${_OPEN_HEADER_DIRS}
        PUBLIC
            ${PLATFORM_PATH}/gd32_os/MSDK/util/include
            ${PLATFORM_PATH}/gd32_os/MSDK/plf/src
            ${PLATFORM_PATH}/gd32_os/MSDK/rtos/rtos_wrapper
    )
    # Set target-specific compile options based on build type
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${TARGET_EXE} PRIVATE -g -O0)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(${TARGET_EXE} PRIVATE -O3 -DNDEBUG)
    elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        target_compile_options(${TARGET_EXE} PRIVATE -O2 -g -DNDEBUG)
    elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        target_compile_options(${TARGET_EXE} PRIVATE -Os -DNDEBUG)
    endif()

    if(ENABLE_ASAN)
        target_compile_options(${TARGET_EXE} PRIVATE -fsanitize=address)
        target_link_libraries(${TARGET_EXE} PRIVATE -fsanitize=address)
    endif()

    target_link_libraries(${TARGET_EXE}
        PRIVATE
            -L${OPEN_LIBS_DIR}
            m               # 仅保留 libm
    )
    # 链接 MSDK_CORE_API 聚合 target，将所有 msdk_agg_api 注册的头文件路径一并传播给编译器
    # （涵盖 bsp_api/fatfs_api/ble_app_profile_api 等），避免逐个手动补路径
    if(TARGET MSDK_CORE_API)
        target_link_libraries(${TARGET_EXE} PRIVATE MSDK_CORE_API)
    endif()
endif()

# Link core gd32_os libs if they were created
if(TARGET ${TARGET_EXE})
    set(_GROUP_LIBS)
    # 方案B：显式使用归档文件路径加入分组，防止目标未被正确分组导致多遍解析缺失。
    # 使用 $<TARGET_FILE:xxx> 获取静态库绝对路径；对 tuyaos_adapter 使用 --whole-archive 保留全部符号。
    set(_GROUP_ARCHIVES)
    # 如果使用裁剪，tuyaos_adapter 为 INTERFACE，实际子库需要显式加入；否则保持原逻辑。
    if(ENABLE_TUYA_TRIM AND TARGET tuyaos_adapter_core)
        set(_ADAPTER_SUBLIBS tuyaos_adapter_core tuyaos_adapter_io tuyaos_adapter_net tuyaos_adapter_init tuyaos_adapter_util)
        if(TARGET tuyaos_adapter_bt)
            list(APPEND _ADAPTER_SUBLIBS tuyaos_adapter_bt)
        endif()
        if(TARGET tuyaos_adapter_sec)
            list(APPEND _ADAPTER_SUBLIBS tuyaos_adapter_sec)
        endif()
        if(TARGET tuyaos_adapter_display)
            list(APPEND _ADAPTER_SUBLIBS tuyaos_adapter_display)
        endif()
    endif()
    set(_ADAPTER_TARGET_LIST tuyaos_adapter)
    if(_ADAPTER_SUBLIBS)
        set(_ADAPTER_TARGET_LIST ${_ADAPTER_SUBLIBS})
    endif()

    foreach(libname tuyaapp tuyaos freertos rtos riscv riscv_api bsp fatfs wifi_mgmt gd32vw55x_peripheral util lwIP mbedtls ble_app_profile ${_ADAPTER_TARGET_LIST})
        if(TARGET ${libname})
            get_target_property(_lib_type ${libname} TYPE)
            if(_lib_type STREQUAL "STATIC_LIBRARY" OR _lib_type STREQUAL "SHARED_LIBRARY" OR _lib_type STREQUAL "MODULE_LIBRARY")
                if(libname MATCHES "^tuyaos_adapter" AND ENABLE_ADAPTER_WHOLE_ARCHIVE)
                    # --whole-archive 需要文件路径；INTERFACE 属性通过下方单独的 target_link_libraries 传播
                    list(APPEND _GROUP_ARCHIVES -Wl,--whole-archive $<TARGET_FILE:${libname}> -Wl,--no-whole-archive)
                else()
                    # 使用目标名而非 $<TARGET_FILE:...>，保留 CMake INTERFACE include/define 传播链
                    list(APPEND _GROUP_ARCHIVES ${libname})
                endif()
            elseif(_lib_type STREQUAL "OBJECT_LIBRARY")
                list(APPEND _GROUP_ARCHIVES $<TARGET_OBJECTS:${libname}>)
            else()
                # INTERFACE / UTILITY 等：直接使用目标名以传递其使用要求
                list(APPEND _GROUP_ARCHIVES ${libname})
            endif()
        endif()
    endforeach()
    if(_GROUP_ARCHIVES)
        # 将 PLATFORM_NEED_LIBS 中的预编译库加入同一链接组，确保与 tuyaos_adapter 等 CMake 库
        # 形成的循环引用可在 --start-group/--end-group 内被正常解析
        if(PLATFORM_NEED_LIBS)
            string(REPLACE " " ";" _PREBUILT_LIBS_LIST "${PLATFORM_NEED_LIBS}")
            foreach(_pl ${_PREBUILT_LIBS_LIST})
                list(APPEND _GROUP_ARCHIVES ${_pl})
            endforeach()
        endif()
        # 将 MSDK/lib/ 中的预编译库同样纳入本组，解决跨组符号引用问题：
        # MSDK/CMakeLists.txt 的 group-1 包含这些库，但 wifi_mgmt/bsp/tuyaapp 等目标
        # 在本 group-2 中，链接器无法跨组回扫。在此重复加入不影响正确性。
        # wifi      : send_to_wifi / wifi_trx_hook_init / wifi_bridge_*
        # rf        : sysctrl_init
        # rftest    : rf test symbols
        # wpas      : wpas_unprot_disconnect / wpas_action_receive / ap_send_deauth
        # ble_max/ble: BLE stack symbols
        if(TARGET ${TARGET_EXE})
            set(_MSDK_PRECOMPILED_LIBS wifi rf rftest wpas)
            if(CONFIG_BLE_FEATURE STREQUAL "MIN")
                list(APPEND _MSDK_PRECOMPILED_LIBS ble)
            else()
                list(APPEND _MSDK_PRECOMPILED_LIBS ble_max)
            endif()
            foreach(_ml ${_MSDK_PRECOMPILED_LIBS})
                list(APPEND _GROUP_ARCHIVES ${_ml})
            endforeach()
        endif()
        # message(STATUS "Link group archives: ${_GROUP_ARCHIVES}")
        # 强制提取 libriscv.a 中的 gd32vw55x_it.o：
        # 向量表引用所有 IRQ handler 均为弱引用，链接器不会自动从静态库中提取。
        # -u USART0_IRQHandler 制造一个强未定义引用，迫使链接器提取 gd32vw55x_it.o，
        # 从而使 BLE/WiFi/UART 等所有中断服务函数均被正确链接（否则向量表条目解析为 0，
        # jalmnxti 跳转到地址 0 触发 Illegal Instruction 异常）。
        target_link_options(${TARGET_EXE} PRIVATE -Wl,-u,USART0_IRQHandler)
        target_link_libraries(${TARGET_EXE} PRIVATE
            $<$<LINK_LANGUAGE:C>:-Wl,--start-group> $<$<LINK_LANGUAGE:CXX>:-Wl,--start-group>
            ${_GROUP_ARCHIVES}
            $<$<LINK_LANGUAGE:C>:-Wl,--end-group> $<$<LINK_LANGUAGE:CXX>:-Wl,--end-group>
        )
        if(ENABLE_TUYA_TRIM)
            target_link_options(${TARGET_EXE} PRIVATE -Wl,--gc-sections)
        endif()
    endif()
endif()

if(TARGET ${TARGET_EXE})
    if(PLATFORM_NEED_LIBS)
        # 已在 --start-group/--end-group 中处理，此处仅保留以备外部覆盖时使用
        # string(REPLACE " " ";" LIBS_LIST "${PLATFORM_NEED_LIBS}")
        # target_link_libraries(${TARGET_EXE} PRIVATE ${LIBS_LIST})
    endif()
endif()
