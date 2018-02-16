#!/bin/sh

lat_ns_="45 70 100 200"
wf_="1 2"
apps_="image randa"
config_="STOCK DIRECT"

build=zynq
adir=$WORKSPACE_LOC/apps
echo $adir

# start with clean build
cd $adir
make build=$build clean

# configure FPGA
cd $adir
make build=$build fpga

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
      make D=$conf,CLOCKS,STATS,T_R=$r_ns,T_W=$w_ns run

    done
  done
done
done
