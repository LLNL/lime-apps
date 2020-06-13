#!/bin/sh

config_=OFFLOAD,SYSTEMC
#,HMCSIM,CONTIG

# larg_=".10 .20 .30 .40 .50 .60 .70 .80 .90"
# harg_=".10 .20 .30 .40 .50 .60 .70 .80 .90 1.00"
# zarg_=".00 .99"
# run_="1 2 3"
larg_=".10 .20 .30 .40 .50 .60 .70 .80 .90"
harg_=".90"
zarg_=".99"
run_="1"

# make application
rm -f *.o *.d rtb makeflags
make D=$config_

for larg in $larg_ ; do
for harg in $harg_ ; do
for zarg in $zarg_ ; do

  args="-e32Mi -l$larg -c -w8Ki -h$harg -z$zarg"
  # args="-e32Mi -l$larg -r$HOME/bio/srr550.fa -w8Ki -h$harg -z$zarg"
  # echo $args

for i in $run_ ; do
  echo "load: $larg hit: $harg zipf: $zarg run: $i"
  # run application
  ./rtb $args

done
done
done
done
