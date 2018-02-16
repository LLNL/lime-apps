@echo off
REM CALL C:\Xilinx\SDK\2017.1\.settings64-Software_Development_Kit__SDK_.bat
CALL C:\Xilinx\SDK\2017.3\.settings64-SDK_Core_Tools.bat
REM This version of GNU tools uses MS-DOS style paths (C:/Users/...)
REM instead of the POSIX equivalent (/root/c/Users/...).
SET PATH=C:\Xilinx\SDK\2017.3\gnuwin\bin;%PATH%
REM WORKSPACE_LOC is used to locate board support packages and hardware platform
SET WORKSPACE_LOC=C:\Users\lloyd23\xwork
SET BOOST_ROOT=C:\cygwin\home\lloyd23\src\boost_1_66_0
