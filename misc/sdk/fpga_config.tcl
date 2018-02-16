# XMD commands to configure the FPGA
set PLAT_DIR [lindex $argv 0]
connect arm hw -cable type xilinx_tcf url TCP:127.0.0.1:3121
# -debugdevice cpunr 1
targets 64
reset_zynqpl
fpga -f $PLAT_DIR/system_wrapper.bit -report_progress
source $PLAT_DIR/ps7_init.tcl
ps7_init
ps7_post_config
xclearzynqresetstatus 64
disconnect 64
