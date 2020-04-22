#!/bin/sh
source /cygdrive/c/Xilinx/Vivado/2018.2/settings64.sh
# Xilinx cross compile tools on Windows expect MS-DOS style (C:/...) absolute paths.
# WORKSPACE_LOC is used to locate board support packages and hardware platform
# export WORKSPACE_LOC=C:/cygwin$HOME/work/lime/standalone/sdk
export BOOST_ROOT=C:/cygwin$HOME/src/boost_1_66_0
