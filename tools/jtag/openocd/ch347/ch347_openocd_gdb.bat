@ECHO OFF
REM
REM

REM Note to use this debug script, the OpenOCD GDBServer and arm-non-eabi-gdb executables
REM needs to be in the system path. If either of these isn't in the system path,
REM OPENOCD_PATH and/or GCC_BIN_PATH must be defined with their location.


 @ECHO ****************************************************************************
 @ECHO    Starting M4 Debug session for FERMION
 @ECHO *****************************************************************************

REM Get the paths from the environment variables if they are set.
SET SERVER_PATH=
SET CLIENT_PATH=
if NOT "%GCC_BIN_PATH%" == "" SET CLIENT_PATH=%GCC_BIN_PATH:"=%\

REM Start the GDB Server.

Taskkill /IM openocd.exe /F 2> nul
START /B openocd.exe -f qcc730.cfg
@ECHO ******************************
@ECHO Running ASIC gdb script
sleep 1
"arm-none-eabi-gdb.exe" -x "gdb.gdbinit"

Taskkill /IM openocd.exe /F 2> nul
