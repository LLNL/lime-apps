#!/bin/sh

config_l=STATS,CLOCKS
#cm2x_l="1 2 4 8 16 32 64"
vault_rns_l="45 70 100 200 400 800 2000 4000 8000"
vault_wf_l="1"
#apps_l="kbtree khash"
adir=$WORKSPACE_LOC/apps
echo $adir

# start with clean build
cd $WORKSPACE_LOC/inia-benchmarks/invertedindex/kmerbench/kbtree/zynq
make clean
cd $WORKSPACE_LOC/inia-benchmarks/invertedindex/kmerbench/khash/zynq
make clean

# configure FPGA
xmd.bat -tcl $adir/misc/sdk-2014_1/fpga_config.tcl $WORKSPACE_LOC/hw_platform_0

#for cm2x in $cm2x_l ; do
#  let "nkmers=524288*cm2x/16/2"
#  let "hsize=nkmers/9"
#  echo "cm2x:$cm2x nkmers:$nkmers hsize:$hsize"
#
#  # update working data set size (-r option) in kmerbench.cpp
#  sed -i -e 's/-r [0-9][0-9]*/-r '$nkmers'/' \
#    -e '/HASHMAP/,/endif/ s/-s [0-9][0-9]*/-s '$hsize'/' \
#    $WORKSPACE_LOC/inia-benchmarks/invertedindex/kmerbench/src/kmerbench.cpp

for v_rns in $vault_rns_l ; do
  for v_wf in $vault_wf_l ; do
    let "v_wns=v_rns*v_wf"
    echo "v_rns:$v_rns v_wns:$v_wns"

    # update timing in clocks.h
    sed -i -e 's/#define T_V_W [0-9][0-9]*/#define T_V_W '$v_wns'/' \
           -e 's/#define T_V_R [0-9][0-9]*/#define T_V_R '$v_rns'/' \
           $adir/shared/clocks.h

if ((1)) ; then
    # make and run kbtree
    cd $WORKSPACE_LOC/inia-benchmarks/invertedindex/kmerbench/kbtree/zynq; rm -f *.o *.elf
    make D=$config_l
    #read -n1 -p "Press [Enter] to continue..."
    xmd.bat -tcl $adir/misc/sdk-2014_1/a9_run.tcl \
      $WORKSPACE_LOC/inia-benchmarks/invertedindex/kmerbench/kbtree/zynq/kmerbench.elf
fi

if ((1)) ; then
    # make and run khash
    cd $WORKSPACE_LOC/inia-benchmarks/invertedindex/kmerbench/khash/zynq; rm -f *.o *.elf
    make D=$config_l
    #read -n1 -p "Press [Enter] to continue..."
    xmd.bat -tcl $adir/misc/sdk-2014_1/a9_run.tcl \
      $WORKSPACE_LOC/inia-benchmarks/invertedindex/kmerbench/khash/zynq/kmerbench.elf
fi

  done
done

#done
