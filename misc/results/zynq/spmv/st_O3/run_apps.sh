#!/bin/bash

lat_ns_="45 70 100 200 400 800 2000 4000 8000"
wf_="1"
apps_="spmv"
config_="STOCK"

# run from apps directory
build=zynq
adir=`pwd`
echo $adir

# start with clean build
cd $adir
make build=$build clean

for sc in {9..15} ; do
for r_ns in $lat_ns_ ; do
for wf in $wf_ ; do
  let w_ns=r_ns*wf
  echo "r_ns: $r_ns w_ns: $w_ns"
  for app in $apps_ ; do
    for conf in $config_ ; do
      echo "app: $app config: $conf"

      # make and run application
      cd $adir/$app/$build
      # use default RUN_ARGS from sources.mak
      # make D=$conf,CLOCKS,STATS,T_R=$r_ns,T_W=$w_ns run
      make D=$conf,CLOCKS,STATS,T_R=$r_ns,T_W=$w_ns RUN_ARGS="-c -s$sc -n41 -v15" run

    done
  done
done
done
done
