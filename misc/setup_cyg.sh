#!/bin/sh
source /cygdrive/c/Xilinx/Vivado/2018.2/settings64.sh
# Xilinx cross compile tools on Windows expect MS-DOS style (C:/...) absolute paths.
# LIME is used to locate shared Makefiles, source, and board support
# export LIME=C:/cygwin$HOME/work/lime
export BOOST_ROOT=C:/cygwin$HOME/src/boost_1_66_0
