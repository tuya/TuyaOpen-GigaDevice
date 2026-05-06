@echo off

REM set custom OpenOCD path
::set OPENOCD_PATH=path/to/openocd/bin
REM set custom Toolchain path
::set TOOLCHAIN_PATH=path/to/toolchain/bin


IF NOT "%1"=="" (
    SET APP=%1
    IF NOT "%2"=="" (
        SET USER_CMD=%2
    ) ELSE (
        SET USER_CMD="-j"
    )
) ELSE (
    SET APP=app
    SET USER_CMD="-j"
)


IF NOT EXIST "%CD%\MSDK\%APP%" (
    echo build app error: %CD%\MSDK\%APP% does not exist !!
    EXIT /B 1
)

REM toolchain check and setup
:: Check for custom toolchain path first
if NOT "%TOOLCHAIN_PATH%"=="" (
    if EXIST "%TOOLCHAIN_PATH%" (
        echo Using custom toolchain path: %TOOLCHAIN_PATH%
        SET "PATH=%PATH%;%TOOLCHAIN_PATH%"
        goto toolchain_done
    )
)

:: Check if toolchain is found in PATH
where riscv-nuclei-elf-gcc >NUL 2>&1

:: Check if toolchain is found in %CD%\tools
if ERRORLEVEL 1 (
    IF NOT EXIST "%CD%\tools\gd32vw55x_toolchain_windows" (
        IF EXIST "%CD%\tools\gd32vw55x_toolchain_windows.7z.001" (
            echo Unzipping gd32vw55x toolchain .......
            "%PROGRAMFILES%\7-Zip\7z.exe" x "%CD%\tools\gd32vw55x_toolchain_windows.7z.001" -o"%CD%\tools"
        ) ELSE (
            echo "Please download the gd32vw55x toolchain from the website and put it in PATH"
            EXIT /B 1
        )
    )
    echo Using toolchain path: %CD%\tools\gd32vw55x_toolchain_windows\bin"
    SET "PATH=%PATH%;%CD%\tools\gd32vw55x_toolchain_windows\bin"
) else (
    for /f "delims=" %%i in ('where riscv-nuclei-elf-gcc') do (
        echo Toolchain found in PATH: %%i
    )
)

:toolchain_done

REM OpenOCD check and setup
:: Check for custom OpenOCD path first
if NOT "%OPENOCD_PATH%"=="" (
    if EXIST "%OPENOCD_PATH%" (
        echo Using custom OpenOCD path: %OPENOCD_PATH%
        SET "PATH=%PATH%;%OPENOCD_PATH%"
        goto openocd_done
    )
)

:: Check if OpenOCD is found in PATH
where openocd >NUL 2>&1

:: Check if OpenOCD is found in %CD%\tools
if ERRORLEVEL 1 (
    IF NOT EXIST "%CD%\tools\xpack-openocd-0.11.0-3_windows" (
        IF EXIST "%CD%\tools\xpack-openocd-0.11.0-3_windows.7z" (
            echo Unzipping gd32vw55x OpenOCD .......
            "%PROGRAMFILES%\7-Zip\7z.exe" x "%CD%\tools\xpack-openocd-0.11.0-3_windows.7z" -o"%CD%\tools"
        ) ELSE (
            echo "Please download the gd32vw55x OpenOCD from the website and put it in PATH"
            EXIT /B 1
        )
    )
    SET "PATH=%PATH%;%CD%\tools\xpack-openocd-0.11.0-3_windows\bin"
) else (
    for /f "delims=" %%i in ('where openocd') do (
        echo OpenOCD found in PATH: %%i
    )
)

:openocd_done


if NOT EXIST cmake_build (
    mkdir cmake_build
)
cd cmake_build

if "%USER_CMD%"=="clean" (
    DEL /S /Q *.* 2>NUL
    FOR /D %%D IN (*.*) DO (
        RD /S /Q "%%D" 2 > NUL
    )
) else (
    :: Configure
    cmake -G "Unix Makefiles" -DAPP=%app% -DCONFIG_BLE_FEATURE=MAX -DCONFIG_MBEDTLS_VERSION="3.6.2" -DCMAKE_TOOLCHAIN_FILE:PATH=%CD%/../scripts/cmake/toolchain.cmake  ..

    :: Make
    make %USER_CMD%
)
cd ..
if ERRORLEVEL 2 pause

:end