#!/bin/bash
# run from apps directory, run_apps.sh <build>
adir=`pwd`
echo pwd: $adir

# build options: arm_64 x86_64 zup zynq (only one)
build=${1:-zup}
echo build: $build

# memory read latency in ns: <integer> (list)
lat_ns_="45 70 100 200"

# memory write latency as factor of read latency: <integer> (list)
wf_="1 2"

# application options: bfs dgemm image pager randa rtb sort spmv strm xsb (list)
apps_="bfs dgemm image pager randa rtb sort spmv strm xsb"

# accelerator options: CLIENT DIRECT OFFLOAD STOCK (list)
accel_="STOCK"

# configuration options: CLOCKS,STATS,TRACE (comma separated)
config=CLOCKS,STATS

# start with clean build
make build=$build clean

# configure FPGA
if [[ $build == "z"* ]]; then
  make build=$build fpga
fi

for r_ns in $lat_ns_ ; do
for wf in $wf_ ; do
  let w_ns=r_ns*wf
  echo "r_ns: $r_ns w_ns: $w_ns"
  for app in $apps_ ; do
    for acc in $accel_ ; do
      echo "app: $app acc: $acc"

      # make and run application
      cd $adir/$app/$build
      # use default RUN_ARGS from sources.mak
      make D=$acc,$config,T_R=$r_ns,T_W=$w_ns run

    done
  done
done
done
