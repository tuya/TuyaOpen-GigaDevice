::     \file    mbl_afterbuild.bat
::     \brief   Bat file for mbl project after build.
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
set TOOLKIT=%1
set ROOT=%2
set ROOT=%ROOT:/=\%
set ALGO_SIGN=%3
set WITH_CERT=%4
set OPENOCD_PATH=%5
set OPENOCD_PATH=%OPENOCD_PATH:/=\%
set AESK=%~6

chcp 65001

set ALGO_HASH=SHA256
set TARGET=mbl

if not '%ALGO_SIGN%' == 'ECDSA256' if not '%ALGO_SIGN%' == 'ED25519' (
    echo ALGO_SIGN must be 'ECDSA256' or 'ED25519'!
    goto end
)

if '%ALGO_SIGN%' == 'ED25519' (
    set KEY_PASSPHRASE=-P "12345678"
) else (
    set KEY_PASSPHRASE=
)

if "%AESK%" NEQ "" (
    set AES_SUFFIX=-aes
) else (
    set AES_SUFFIX=
)

set MBL_KEY=%ROOT%\scripts\certs\%ALGO_SIGN%\mbl-key.pem
set ROTPK=%ROOT%\scripts\certs\%ALGO_SIGN%\rot-key.pem
set MBL_CERT=%ROOT%\scripts\certs\%ALGO_SIGN%\mbl-cert.pem
set CONFIG_FILE=..\..\config\config_gdm32.h
set SYSTOOL=%ROOT%\scripts\imgtool\sysset.py
set IMGTOOL=%ROOT%\scripts\imgtool\imgtool.py
set HEXTOOL=%ROOT%\scripts\imgtool\hextool.py
set GENTOOL=%ROOT%\scripts\imgtool\gentool.py
set AESTOOL=%ROOT%\scripts\imgtool\aestool.py
set SREC_CAT=%ROOT%\scripts\imgtool\srec_cat.exe
set OUTPUT_PATH=%ROOT%\\scripts\\images\\
set DOWNLOAD_BIN=%OUTPUT_PATH%\\mbl-sys%AES_SUFFIX%.bin

:: Generate dump and bin file
if "%TOOLKIT%" neq "IAR" (
    %TOOLKIT%objdump -d %TARGET%.elf >  %TARGET%.dump
    %TOOLKIT%objcopy -O binary %TARGET%.elf  %TARGET%.bin
)

IF EXIST "%OUTPUT_PATH%\%TARGET%.bin"  del "%OUTPUT_PATH%\%TARGET%.bin"
IF EXIST "%DOWNLOAD_BIN%" del "%DOWNLOAD_BIN%"

:: find RE_MBL_OFFSET defined in CONFIG_FILE
set mbl_offset=0x0
for /f "tokens=1,2,3" %%i in ( %ROOT%\config\config_gdm32.h ) do (
    if "%%j" == "RE_MBL_OFFSET" (
        set mbl_offset=%%k
    )
)
:: Check if need python to add sysset\mbl header\mbl tailer
if "%mbl_offset%" == "0x0"  (
    echo "Not add image header and tailer, goto download!"
	copy %TARGET%.bin "%OUTPUT_PATH%\%TARGET%.bin"
	set DOWNLOAD_BIN=%TARGET%.bin
    goto download
)

:: Print ROTPK HASH
::python %IMGTOOL% getpub -k %ROTPK%  %KEY_PASSPHRASE%  --sha256 1

:: Generate system setting hex
python %SYSTOOL% -t "SYS_SET" -c %CONFIG_FILE% %OUTPUT_PATH%\sysset.bin

:: Generate system status hex (padding with 0xFF)
:: python %SYSTOOL% -t "SYS_STATUS" -c %CONFIG_FILE%  %OUTPUT_PATH%\sysstatus.bin

IF EXIST "%OUTPUT_PATH%\mbl-sign.bin"  del "%OUTPUT_PATH%\mbl-sign.bin"

:: Add image header, ptlvs and concatenate the cert
copy %TARGET%.bin "%OUTPUT_PATH%\%TARGET%.bin"

if '%WITH_CERT%' == 'CERT' (
    python %IMGTOOL% sign --config %CONFIG_FILE% ^
                      -k %MBL_KEY% ^
                      %KEY_PASSPHRASE% ^
                      -t "MBL" ^
                      --algo_hash "%ALGO_HASH%" ^
                      --algo_sig "%ALGO_SIGN%" ^
                      --cert %MBL_CERT% ^
                      --cert_key %ROTPK% ^
                      %TARGET%.bin %OUTPUT_PATH%\mbl-sign.bin
) else (
    python %IMGTOOL% sign --config %CONFIG_FILE% ^
                      -k %ROTPK% ^
                      %KEY_PASSPHRASE% ^
                      -t "MBL" ^
                      --algo_hash "%ALGO_HASH%" ^
                      --algo_sig "%ALGO_SIGN%" ^
                      %TARGET%.bin %OUTPUT_PATH%\mbl-sign.bin
)

python %GENTOOL% --config %CONFIG_FILE% ^
                 --sys_set %OUTPUT_PATH%\sysset.bin ^
                 --mbl %OUTPUT_PATH%\mbl-sign.bin ^
                 -o %OUTPUT_PATH%\mbl-sys.bin
IF EXIST "%OUTPUT_PATH%\sysset.bin" del "%OUTPUT_PATH%\sysset.bin"
IF EXIST "%OUTPUT_PATH%\mbl-sign.bin" del "%OUTPUT_PATH%\mbl-sign.bin"

if "%AESK%" == ""  (
    python %HEXTOOL% -c %CONFIG_FILE% ^
            -t "SYS_SET" ^
            -e %SREC_CAT% ^
            %OUTPUT_PATH%\mbl-sys.bin ^
            %OUTPUT_PATH%\mbl-sys.hex
)  else (
    python %IMGTOOL% pad -s 0x8000 ^
                         %OUTPUT_PATH%\mbl-sys.bin %OUTPUT_PATH%\mbl-sys-pad.bin
    python %AESTOOL% --c %CONFIG_FILE%   ^
            -t "SYS_SET" ^
            -i %OUTPUT_PATH%\mbl-sys-pad.bin ^
            -o %OUTPUT_PATH%\mbl-sys%AES_SUFFIX%.bin ^
            -k %AESK%
::    python %HEXTOOL% -c %CONFIG_FILE% ^
::            -t "SYS_SET" ^
::            -e %SREC_CAT% ^
::            %OUTPUT_PATH%\mbl-sys%AES_SUFFIX%.bin ^
::            %OUTPUT_PATH%\mbl-sys.hex
    del "%OUTPUT_PATH%\mbl-sys-pad.bin"

    echo Encrypted!
)

:download
set OPENOCD="%OPENOCD_PATH%\\openocd.exe"
::set LINKCFG="%CD%\\..\\openocd_gdlink.cfg"
set LINKCFG="%CD%\\..\\openocd_jlink.cfg"
@echo on
::%OPENOCD% -f %LINKCFG% -c "program %DOWNLOAD_BIN% 0x08000000 verify exit;"
:end
