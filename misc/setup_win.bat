@echo off
CALL C:\Xilinx\Vivado\2018.2\settings64.sh
REM This version of GNU tools uses MS-DOS style paths (C:/Users/...)
REM instead of the POSIX equivalent (/root/c/Users/...).
SET PATH=C:\Xilinx\SDK\2018.2\gnuwin\bin;%PATH%
REM LIME is used to locate shared Makefiles, source, and board support
REM SET LIME=%HOMEDRIVE%%HOMEPATH%\work\lime
SET BOOST_ROOT=%HOMEDRIVE%%HOMEPATH%\src\boost_1_66_0
