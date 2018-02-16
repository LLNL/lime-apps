#!/bin/sh

config=STATS,CLOCKS
#cm2x_="1 2 4 8 16 32 64"
lat_ns_="45 70 100 200 400 800 2000 4000 8000"
wf_="1"
#apps_="kbtree khash"

adir=$WORKSPACE_LOC/apps
echo $adir

# start with clean build
cd $WORKSPACE_LOC/inia-benchmarks/invertedindex/kmerbench/kbtree/zynq
make clean
cd $WORKSPACE_LOC/inia-benchmarks/invertedindex/kmerbench/khash/zynq
make clean

# configure FPGA
xmd.bat -tcl $adir/misc/sdk/fpga_config.tcl $WORKSPACE_LOC/hw_platform_0

#for cm2x in $cm2x_ ; do
#  let "nkmers=524288*cm2x/16/2"
#  let "hsize=nkmers/9"
#  echo "cm2x:$cm2x nkmers:$nkmers hsize:$hsize"
#
#  # update working data set size (-r option) in kmerbench.cpp
#  sed -i -e 's/-r [0-9][0-9]*/-r '$nkmers'/' \
#    -e '/HASHMAP/,/endif/ s/-s [0-9][0-9]*/-s '$hsize'/' \
#    $WORKSPACE_LOC/inia-benchmarks/invertedindex/kmerbench/src/kmerbench.cpp

for r_ns in $lat_ns_ ; do
  for wf in $wf_ ; do
    let "w_ns=r_ns*wf"
    echo "r_ns: $r_ns w_ns: $w_ns"

if ((1)) ; then
    # make and run kbtree
    cd $WORKSPACE_LOC/inia-benchmarks/invertedindex/kmerbench/kbtree/zynq; rm -f *.o *.elf
    make D=$config,T_R=$r_ns,T_W=$w_ns
    #read -n1 -p "Press [Enter] to continue..."
    xmd.bat -tcl $adir/misc/sdk/a9_run.tcl \
      $WORKSPACE_LOC/inia-benchmarks/invertedindex/kmerbench/kbtree/zynq/kmerbench.elf
fi

if ((1)) ; then
    # make and run khash
    cd $WORKSPACE_LOC/inia-benchmarks/invertedindex/kmerbench/khash/zynq; rm -f *.o *.elf
    make D=$config,T_R=$r_ns,T_W=$w_ns
    #read -n1 -p "Press [Enter] to continue..."
    xmd.bat -tcl $adir/misc/sdk/a9_run.tcl \
      $WORKSPACE_LOC/inia-benchmarks/invertedindex/kmerbench/khash/zynq/kmerbench.elf
fi

  done
done

#done
