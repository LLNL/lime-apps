make D=CLIENT,CLOCKS,STATS
xmd.bat -tcl ../../misc/sdk-2016_3/fpga_config.tcl ../../../hw_platform_0
xmd.bat -tcl ../../misc/sdk-2016_3/mb_start.tcl ../mcu/rtb_mcu.elf
xmd.bat -tcl ../../misc/sdk-2016_3/a9_run.tcl rtb.elf
