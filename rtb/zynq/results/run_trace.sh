#!/bin/sh

config_="USE_HASH"
# latency R,W
lat_ns_="85,106 200,400"
larg_=".10 .50 .90"
harg_=".90"
zarg_=".99"

# config_="STL DIRECT USE_HASH"
# latency R,W
# lat_ns_="85,106 200,400"
# larg_=".10 .20 .30 .40 .50 .60 .70 .80 .90"
# harg_=".10 .20 .30 .40 .50 .60 .70 .80 .90"
# zarg_=".00 .99"

sed=C:/cygwin/bin/sed.EXE
adir=$WORKSPACE_LOC/apps
echo $adir

# start with clean build
#make build=zynq clean

# configure FPGA
xmd.bat -tcl $adir/misc/sdk-2016_3/fpga_config.tcl $WORKSPACE_LOC/hw_platform_0

for lat_ns in $lat_ns_ ; do
  v_rns=${lat_ns%%,*}
  v_wns=${lat_ns##*,}
  echo "v_rns: $v_rns v_wns: $v_wns"

  # update timing in clocks.h
  $sed -i -e "s/#define T_V_R[ ]\{1,\}[0-9]\{1,\}/#define T_V_R $v_rns/" \
          -e "s/#define T_V_W[ ]\{1,\}[0-9]\{1,\}/#define T_V_W $v_wns/" \
          $adir/shared/clocks.h

for larg in $larg_ ; do
for harg in $harg_ ; do
for zarg in $zarg_ ; do
for conf in $config_ ; do
  echo "load: $larg hit: $harg zipf: $zarg config: $conf"

  args="#define ARGS (char*)\"-e32Mi\", (char*)\"-l$larg\", (char*)\"-rsrr550.fa\", (char*)\"-w1Mi\", (char*)\"-h$harg\", (char*)\"-z$zarg\""
  # echo $args

  # update arguments
  $sed -i -e "/^#define ARGS/ c\\$args" $adir/rtb/src/rtb.cpp

  # make and run application
  cd $adir/rtb/zynq; rm -f rtb.o rtb.elf
  make D=$conf,CLOCKS,STATS,TRACE,USE_SD
  xmd.bat -tcl $adir/misc/sdk-2016_3/a9_run.tcl $adir/rtb/zynq/rtb.elf

done
done
done
done

done
