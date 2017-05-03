#!/bin/sh

config_=DIRECT

larg_=".20 .60 .90"
harg_=".82 .90 .98"
zarg_=".00 .99"
run_="1 2 3"

# make application
rm -f *.o rtb
make D=$config_

for larg in $larg_ ; do
for harg in $harg_ ; do
for zarg in $zarg_ ; do

  args="-e32Mi -l$larg -r$HOME/bio/SRR000550.fa -w8Mi -h$harg -z$zarg"
  # echo $args

for i in $run_ ; do
  echo "load: $larg hit: $harg zipf: $zarg run: $i"
  # run application
  ./rtb $args

done
done
done
done
