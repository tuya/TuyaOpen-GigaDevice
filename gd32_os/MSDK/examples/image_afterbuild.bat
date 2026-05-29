::     \file    image_afterbuild.bat
::     \brief   Bat file for image after build.
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
chcp 65001

set TOOLKIT=%1
set OPENOCD_PATH=%2
set ROOT=%3
set TARGET=%4

set CONFIG_FILE=%ROOT%\config\config_gdm32.h
set IMGTOOL=%ROOT%\scripts\imgtool\imgtool.py
set HEXTOOL=%ROOT%\scripts\imgtool\hextool.py
set GENTOOL=%ROOT%\scripts\imgtool\gentool.py
set AESTOOL=%ROOT%\scripts\imgtool\aestool.py
set SREC_CAT=%ROOT%\scripts\imgtool\srec_cat.exe
set INPUT_PATH=%ROOT%\scripts\images\
set OUTPUT_PATH=%CD%\..\image
set cur_dir=%CD%

mkdir %OUTPUT_PATH%

if "%TOOLKIT%" neq "IAR" (
    %TOOLKIT%objdump -S -l -d %TARGET%.elf > %TARGET%.dump
    %TOOLKIT%objcopy -O binary --remove-section ".log" %TARGET%.elf %TARGET%.bin
)

IF EXIST %OUTPUT_PATH%\image-ota.bin  del %OUTPUT_PATH%\image-ota.bin

:: find RE_MBL_OFFSET defined in CONFIG_FILE
set mbl_offset=0x0
set image0_offset=0x0
set image1_offset=0x0

for /f "tokens=1,2,3" %%i in ( %CONFIG_FILE% ) do (
    if "%%j" == "RE_MBL_OFFSET" (
        set mbl_offset=%%k
    )
    if "%%j" == "RE_IMG_0_OFFSET" (
        set image0_offset=%%k
    )
    if "%%j" == "RE_IMG_1_OFFSET" (
        set image1_offset=%%k
    )
)

echo mbl_offset=%mbl_offset% image0_offset=%image0_offset% image1_offset=%image1_offset%

set cur_dir=%CD%

cd %INPUT_PATH%
if exist mbl.bin (
    for /f %%i in ('dir /b mbl.bin') do (
        set mbl_len=%%~zi
    )
)
echo mbl_len = %mbl_len%

cd %cur_dir%
if "%mbl_offset%" == "0x0"  (
    echo "Not add image header and tailer!"
	copy %TARGET%.bin "%OUTPUT_PATH%\\image-ota.bin" /Y

    if exist "%INPUT_PATH%\\mbl.bin" (
        %SREC_CAT% "%INPUT_PATH%\mbl.bin" -Binary -offset "0" ^
                 %TARGET%.bin -Binary -offset "%image0_offset%" ^
                 -fill 0xFF %mbl_len% "%image0_offset%" ^
                 -o "%OUTPUT_PATH%\image-all.bin" -Binary
    )
)