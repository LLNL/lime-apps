#!/bin/sh

config_=DIRECT,PSL_HISTO

earg_="32 16 8"
larg_=".10 .20 .30 .40 .50 .60 .70 .80 .90"
# earg_="32"
# larg_=".10 .50 .90"

# make application
rm -f *.o rtb
make D=$config_

for earg in $earg_ ; do
for larg in $larg_ ; do

  args="-e${earg}Mi -l$larg -r$HOME/bio/SRR000550.fa"
  # echo $args

echo "entries: $earg load: $larg"
# run application
./rtb $args

done
done
