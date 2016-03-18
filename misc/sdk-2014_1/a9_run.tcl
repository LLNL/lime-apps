# XMD commands to run a program on the ARM A9
set PROG_ELF [lindex $argv 0]
connect arm hw -cable type xilinx_tcf url TCP:127.0.0.1:3121 -debugdevice cpunr 1
targets 64
rst -processor
dow $PROG_ELF
safemode off
bpr all
bps exit
con -block
disconnect 64
