#!/bin/sh

# latency R,W
lat_ns_="85,106 200,400"
larg_=".10 .50 .90"
harg_=".90"
zarg_=".99"
config_="USE_HASH"

# latency R,W
# lat_ns_="85,106 200,400"
# larg_=".10 .20 .30 .40 .50 .60 .70 .80 .90"
# harg_=".10 .20 .30 .40 .50 .60 .70 .80 .90"
# zarg_=".00 .99"
# config_="STL DIRECT USE_HASH"

build=zynq
adir=$WORKSPACE_LOC/apps
echo $adir

# start with clean build
cd $adir
make build=$build clean

# configure FPGA
cd $adir
make build=$build fpga

for lat_ns in $lat_ns_ ; do
  r_ns=${lat_ns%%,*}
  w_ns=${lat_ns##*,}
  echo "r_ns: $r_ns w_ns: $w_ns"

  for larg in $larg_ ; do
  for harg in $harg_ ; do
  for zarg in $zarg_ ; do
  for conf in $config_ ; do
    echo "load: $larg hit: $harg zipf: $zarg config: $conf"

    # make and run application
    cd $adir/rtb/$build
    args="-e32Mi -l$larg -rsrr550.fa -w1Mi -h$harg -z$zarg"
    make D=$conf,CLOCKS,STATS,T_R=$r_ns,T_W=$w_ns RUN_ARGS="$args" run

  done
  done
  done
  done

done
