::     \file    mbl_prebuild.bat
::     \brief   Bat file for mbl project prebuild.
::
::     \version 2023-07-20, V1.0.0, firmware for GD32VW55x
::
::     Copyright (c) 2023, GigaDevice Semiconductor Inc.
::
::     Redistribution and use in source and binary forms, with or without modification,
:: are permitted provided that the following conditions are met:
::
::     1. Redistributions of source code must retain the above copyright notice, this
::        list of conditions and the following disclaimer.
::     2. Redistributions in binary form must reproduce the above copyright notice,
::        this list of conditions and the following disclaimer in the documentation
::        and/or other materials provided with the distribution.
::     3. Neither the name of the copyright holder nor the names of its contributors
::        may be used to endorse or promote products derived from this software without
::        specific prior written permission.
::
::     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
:: AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
:: WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
:: IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
:: INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
:: NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
:: PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
:: WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
:: ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
:: OF SUCH DAMAGE.

@echo off

set CROSS_PREFIX=%1
chcp 65001

echo cd=%cd%

:: generate mbl.lds
if "%CROSS_PREFIX%" neq "IAR" (
    %CROSS_PREFIX%gcc -E -P -o ..\mbl.lds -x c-header ..\mbl.ld -I ..\..\..\mainboot -I ..\..\..\..\config
)

::set MBL_DIR=..\\..\\..
::set SCONS_CONFIG=%MBL_DIR%\\..\\MSDK\\macsw\\config\\wlan_config.h
::set PLATFORM_CONFIG=..\..\..\..\config\platform_def.h

:: use different rom_symbol.gcc for FPGA_V7 and FPGA_Ultra and ASIC b_cut
::set combo_en=1
::findstr /r /c:"#define[ ]*CONFIG_PLATFORM[ ]*PLATFORM_FPGA_32103_V7" %PLATFORM_CONFIG% && set combo_en=0
::set asic=1
::findstr /r /c:"#define[ ]*CONFIG_PLATFORM[ ]*PLATFORM_FPGA[0-9A-Za-z_]*" %PLATFORM_CONFIG% && set asic=0
::set a_cut=0
::findstr /r /c:"#define[ ]*ASIC_CUT[ ]*A_CUT" %PLATFORM_CONFIG% && set a_cut=1
::
::if "%asic%" == "1" (
::    if "%a_cut%" == "1" (
::        echo copy rom_symbol_acut.gcc rom_symbol.gcc
::        copy %MBL_DIR%\\..\\ROM-EXPORT\\symbol\\rom_symbol_acut.gcc  %MBL_DIR%\\..\\ROM-EXPORT\\symbol\\rom_symbol.gcc
::    ) else if "%a_cut%" == "0" (
::        echo copy rom_symbol_bcut.gcc rom_symbol.gcc
::        copy %MBL_DIR%\\..\\ROM-EXPORT\\symbol\\rom_symbol_bcut.gcc  %MBL_DIR%\\..\\ROM-EXPORT\\symbol\\rom_symbol.gcc
::    )
::) else  (
::    if "%combo_en%" == "1" (
::        echo copy rom_symbol_ultra.gcc rom_symbol.gcc
::        copy %MBL_DIR%\\..\\ROM-EXPORT\\symbol\\rom_symbol_ultra.gcc  %MBL_DIR%\\..\\ROM-EXPORT\\symbol\\rom_symbol.gcc
::    ) else if "%combo_en%" == "0" (
::        echo copy rom_symbol_v7.gcc rom_symbol.gcc
::        copy %MBL_DIR%\\..\\ROM-EXPORT\\symbol\\rom_symbol_v7.gcc  %MBL_DIR%\\..\\ROM-EXPORT\\symbol\\rom_symbol.gcc
::    )
::)
:end
