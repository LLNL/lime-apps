# XMD commands to start a program on the MicroBlaze
set PROG_ELF [lindex $argv 0]
connect mb mdm -cable type xilinx_tcf url TCP:127.0.0.1:3121 -debugdevice cpunr 1
targets 0
rst -processor
dow $PROG_ELF
safemode off
bpr all
bps exit
con
disconnect 0
