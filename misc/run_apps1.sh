#!/bin/sh

config_l=STATS,CLOCKS
vault_rns_l="40 400 4000"
vault_wf_l="1"
scale_l="15 16 17 18 19 20"
apps_l="sort"
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

for sc in $scale_l ; do
  echo "scale:$sc"

  # update working data set size (-s option)
  sed -i -e '/Arguments beg/,/Arguments end/ s/-s[0-9][0-9]*/-s'$sc'/' \
    $adir/sort/src/sort.cpp

    # make and run sort
    cd $adir/sort/zynq; rm -f *.o *.elf
    make D=$config_l
    xmd.bat -tcl $adir/misc/sdk-2014_1/a9_run.tcl $adir/sort/zynq/sort.elf

done

  done
done
