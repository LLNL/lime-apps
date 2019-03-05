@echo off
CALL C:\Xilinx\Vivado\2018.2\settings64.sh
REM This version of GNU tools uses MS-DOS style paths (C:/Users/...)
REM instead of the POSIX equivalent (/root/c/Users/...).
SET PATH=C:\Xilinx\SDK\2018.2\gnuwin\bin;%PATH%
REM WORKSPACE_LOC is used to locate board support packages and hardware platform
SET WORKSPACE_LOC=%HOMEDRIVE%%HOMEPATH%\work\lime\standalone\sdk
SET BOOST_ROOT=%HOMEDRIVE%%HOMEPATH%\src\boost_1_66_0
