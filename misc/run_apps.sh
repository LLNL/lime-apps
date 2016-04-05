#!/bin/sh

config_l=STATS,CLOCKS
vault_rns_l="45 70 100 200"
vault_wf_l="1 2"
apps_l="image randa"
adir=$WORKSPACE_LOC/apps
echo $adir

# start with clean build
make build=zynq clean

# configure FPGA
xmd.bat -tcl $adir/misc/sdk-2014_1/fpga_config.tcl $WORKSPACE_LOC/hw_platform_0

for v_rns in $vault_rns_l ; do
  for v_wf in $vault_wf_l ; do
    let v_wns=v_rns*v_wf
    echo "v_rns:$v_rns v_wns:$v_wns"

    # update timing in clocks.h
    sed -i -e 's/#define T_V_W [0-9][0-9]*/#define T_V_W '$v_wns'/' \
           -e 's/#define T_V_R [0-9][0-9]*/#define T_V_R '$v_rns'/' \
           $adir/shared/clocks.h

    # make and run image
    cd $adir/image/zynq; rm -f *.o *.elf
    make D=$config_l
    xmd.bat -tcl $adir/misc/sdk-2014_1/a9_run.tcl $adir/image/zynq/image.elf

    # make and run randa
    cd $adir/randa/zynq; rm -f *.o *.elf
    make D=$config_l
    xmd.bat -tcl $adir/misc/sdk-2014_1/a9_run.tcl $adir/randa/zynq/randa.elf

  done
done
