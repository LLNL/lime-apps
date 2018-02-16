make D=CLIENT,CLOCKS,STATS
xmd.bat -tcl ../../misc/sdk/fpga_config.tcl ../../../hw_platform_0
xmd.bat -tcl ../../misc/sdk/mb_start.tcl ../mcu/$1.elf
xmd.bat -tcl ../../misc/sdk/a9_run.tcl $1.elf
