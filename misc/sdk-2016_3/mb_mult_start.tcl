# XMD commands to start a program on the MicroBlaze
# This is a modification of mb_start to support multiple MicroBlazes.  MicroBlazes are numbered 0 through n.
# Use example (to load Test_loopBack_mb0.elf into MicroBlaze 0 and run the program): 
# xmd.bat -tcl misc/sdk-2016_3/mb_mult_start.tcl ../../../sys_ps7_3uB/sys_ps7.sdk/Test_LoopBack_mb0/Debug/Test_LoopBack_mb0.elf 0

set PROG_ELF [lindex $argv 0]

set MB_NUM   [lindex $argv 1]
set CPU_NUM  [expr {$MB_NUM + 1}]

connect mb mdm -cable type xilinx_tcf url TCP:127.0.0.1:3121 -debugdevice cpunr $CPU_NUM
targets $MB_NUM
rst -processor
dow $PROG_ELF
safemode off
bpr all
bps exit
con
disconnect $MB_NUM
