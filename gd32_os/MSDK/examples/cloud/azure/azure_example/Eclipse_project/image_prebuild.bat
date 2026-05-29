::     \file    image_prebuild.bat
::     \brief   Bat file for image prebuild.
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

@echo on
chcp 65001

set TOOLKIT=%1
set MSDK_DIR=%2
set MATTER=%4

set MSDK_DIR=..\..\..\..\..\..
set SCONS_CONFIG=%MSDK_DIR%\macsw\export\wlan_config.h
set PLATFORM_CONFIG=%MSDK_DIR%\..\config\platform_def.h

echo %MSDK_DIR%
echo cd=%cd%

:: generate gd32vw55x.lds
if "%TOOLKIT%" neq "IAR" (
    if "%MATTER%" == "matter" (
        %TOOLKIT%gcc -E -DCFG_MATTER -P -o  %MSDK_DIR%\plf\riscv\env\gd32vw55x.lds ^
                        -x c-header %MSDK_DIR%\plf\riscv\env\gd32vw55x.ld ^
                        -I %MSDK_DIR%\..\config -I %MSDK_DIR%\..\MBL\mainboot ^
                        -I %MSDK_DIR%\macsw\export -I %MSDK_DIR%\util\include
        ) else (
        %TOOLKIT%gcc -E -P -o %MSDK_DIR%\plf\riscv\env\gd32vw55x.lds ^
                        -x c-header %MSDK_DIR%\plf\riscv\env\gd32vw55x.ld ^
                        -I %MSDK_DIR%\..\config -I %MSDK_DIR%\..\MBL\mainboot ^
                        -I %MSDK_DIR%\macsw\export -I %MSDK_DIR%\util\include ^
                        -I %MSDK_DIR%\app
        )
)

:::: use different rom_symbol.gcc for FPGA_V7 and FPGA_Ultra and ASIC b_cut
::set combo_en=1
:::: findstr /r /c:"#define[ ]*CONFIG_PLATFORM[ ]*PLATFORM_FPGA_32103_V7" %PLATFORM_CONFIG% && set combo_en=0
::set asic=1
::findstr /r /c:"^#define\s*CONFIG_PLATFORM[ ]*[A-Za-z_]FPGA[A-Za-z_]$" %PLATFORM_CONFIG% && set asic=0
::set a_cut=0
::findstr /r /c:"#define[ ]*ASIC_CUT[ ]*A_CUT" %PLATFORM_CONFIG% && set a_cut=1
::
::if "%asic%" == "1" (
::    if "%a_cut%" == "1" (
::        echo copy rom_symbol_acut.gcc rom_symbol.gcc
::        copy %MSDK_DIR%\\..\\ROM-EXPORT\\symbol\\rom_symbol_acut.gcc  %MSDK_DIR%\\..\\ROM-EXPORT\\symbol\\rom_symbol.gcc
::    ) else if "%a_cut%" == "0" (
::        echo copy rom_symbol_bcut.gcc rom_symbol.gcc
::        copy %MSDK_DIR%\\..\\ROM-EXPORT\\symbol\\rom_symbol_bcut.gcc  %MSDK_DIR%\\..\\ROM-EXPORT\\symbol\\rom_symbol.gcc
::    )
::) else  (
::    if "%combo_en%" == "1" (
::        echo copy rom_symbol_ultra.gcc rom_symbol.gcc
::        copy %MSDK_DIR%\\..\\ROM-EXPORT\\symbol\\rom_symbol_ultra.gcc  %MSDK_DIR%\\..\\ROM-EXPORT\\symbol\\rom_symbol.gcc
::    ) else if "%combo_en%" == "0" (
::        echo copy rom_symbol_v7.gcc rom_symbol.gcc
::        copy %MSDK_DIR%\\..\\ROM-EXPORT\\symbol\\rom_symbol_v7.gcc  %MSDK_DIR%\\..\\ROM-EXPORT\\symbol\\rom_symbol.gcc
::    )
::)

chcp 437
echo /* Do not change the content here, it's auto generated */ > %MSDK_DIR%\app\_build_date.h

REM build date
del %MSDK_DIR%\examples\cloud\azure\azure_example\src\_build_date.h
echo #define SDK_BUILD_DATE "%DATE:~-10% %TIME:~0,-3%" >> %MSDK_DIR%\examples\cloud\azure\azure_example\src\_build_date.h

copy %MSDK_DIR%\app\cmd_shell.h %MSDK_DIR%\examples\cloud\azure\azure_example\src\cmd_shell.h
copy %MSDK_DIR%\app\ping.h %MSDK_DIR%\examples\cloud\azure\azure_example\src\ping.h
copy %MSDK_DIR%\app\atcmd.h %MSDK_DIR%\examples\cloud\azure\azure_example\src\atcmd.h
copy %MSDK_DIR%\app\atcmd.c %MSDK_DIR%\examples\cloud\azure\azure_example\src\atcmd.c

copy %MSDK_DIR%\app\atcmd_azure.h %MSDK_DIR%\examples\cloud\azure\azure_example\src\atcmd_azure.h
copy %MSDK_DIR%\app\atcmd_azure.c %MSDK_DIR%\examples\cloud\azure\azure_example\src\atcmd_azure.c

copy %MSDK_DIR%\app\atcmd_tcpip.h %MSDK_DIR%\examples\cloud\azure\azure_example\src\atcmd_tcpip.h
copy %MSDK_DIR%\app\atcmd_tcpip.c %MSDK_DIR%\examples\cloud\azure\azure_example\src\atcmd_tcpip.c

copy %MSDK_DIR%\app\atcmd_wifi.h %MSDK_DIR%\examples\cloud\azure\azure_example\src\atcmd_wifi.h
copy %MSDK_DIR%\app\atcmd_wifi.c %MSDK_DIR%\examples\cloud\azure\azure_example\src\atcmd_wifi.c
goto end

:: generate trace file ids
python ..\\..\\gen_trace_id.py ..\\..\\msdk\\msdk_out

:end
