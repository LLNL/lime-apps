# XMD commands to stop a program on the MicroBlaze
connect mb mdm -cable type xilinx_tcf url TCP:127.0.0.1:3121 -debugdevice cpunr 1
targets 0
stop
disconnect 0
