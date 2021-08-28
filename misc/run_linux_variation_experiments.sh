# This file runs the elf files that are generated and copied the boot partition of the SD card
# using prep_linux_variation_experiments.sh 
# The run output should be piped to a text file, which can be passed as an argument using 
# misc/extract_runtimes.sh and plotted using misc/plots_linux.m

# Parameters to sweep
div_="4 8 16 32"
resps_="106W85R 400W200R"

# Table or PwCLT based generators. Preferably pass in commandline
# gen="pwclt"
# gen="table"
gen=$1

# This is mounted boot partition (should be mounted before the runs)
sdcard="/mnt/sdb1"

# Sweep through the parameters
for resp in $resps_ ; do
  for div in $div_ ; do
    echo "----------------------------------------------------------------"
    echo "Now running the experiments with generator latency div =" $gen $resp $div
    echo "----------------------------------------------------------------"

    # Do benchmark runs
    ds_="4 8 16 32 64"
    for d in $ds_ ; do
      $sdcard/image_$gen$resp$div$d.elf -d$d -v15 -w24000 -h16000 pat pat
    done
    # $sdcard/pager_$gen$resp$div.elf
    $sdcard/spmv_$gen$resp$div.elf
    $sdcard/randa_$gen$resp$div.elf
    $sdcard/rtb_$gen$resp$div.elf -e32Mi -l.60 -c -w1Mi -h.90 -z.99
    $sdcard/strm_$gen$resp$div.elf
    $sdcard/xsb_$gen$resp$div.elf -s small
  done
done
