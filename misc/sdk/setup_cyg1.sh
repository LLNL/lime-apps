#!/bin/sh
source /cygdrive/c/Xilinx/SDK/2017.1/.settings64-Software_Development_Kit__SDK_.sh
# source /cygdrive/c/Xilinx/SDK/2017.3/.settings64-SDK_Core_Tools.sh
# Xilinx cross compile tools on Windows expect MS-DOS style (C:/...) absolute paths.
# WORKSPACE_LOC is used to locate board support packages and hardware platform
export WORKSPACE_LOC=C:/Users/lloyd23/xwork
export BOOST_ROOT=C:/cygwin/home/lloyd23/src/boost_1_66_0
