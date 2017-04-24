# XMD commands to stop a program on the MicroBlaze
# This is a modification of mb_stop to support multiple MicroBlazes.  MicroBlazes are numbered 0 through n.
# Use example (to stop a program running on MicroBlaze 2): 
# xmd.bat -tcl misc/sdk-2016_3/mb_mult_stop.tcl 2

set MB_NUM   [lindex $argv 0]
set CPU_NUM  [expr {$MB_NUM + 1}]

connect mb mdm -cable type xilinx_tcf url TCP:127.0.0.1:3121 -debugdevice cpunr $CPU_NUM
targets $MB_NUM
stop
disconnect $MB_NUM
