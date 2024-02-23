@ECHO OFF
REM
REM

REM Get the paths from the environment variables if they are set.
SET SERVER_PATH=
SET CLIENT_PATH=
if NOT "%GCC_BIN_PATH%" == "" SET CLIENT_PATH=%GCC_BIN_PATH:"=%\

REM Use OpenOCD to load image to RRAM

Taskkill /IM openocd.exe /F 2> nul
START /B openocd.exe -f qcc730_ftdi_img.cfg
::Taskkill /IM openocd.exe /F 2> nul
