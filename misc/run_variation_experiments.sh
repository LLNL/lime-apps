# Parameters to sweep
div_="4 8 16 32"
resps_="106W85R 400W200R"

# Table or PwCLT based generators. (Un)comment accordingly
# generator="table"
generator="pwclt"

# Individual channel latency clock cycles (mean of Gaussians)
lats_106_85_="216 216 636 510 72 72 492 366"
lats_400_200_="216 216 2400 1200 72 72 2256 1056"

# This is the FPGA serial port device to listen to
serialport="/dev/ttyS3"

# Program FPGA
make build=zup fpga
sleep 5

# Clear the earlier serial port log & kill bg screen process if exists and 
pkill screen
rm -rf screenlog.0

# Start listening to the serialport in background
screen -dm -L $serialport 115200

# Sweep through the parameters
for resp in $resps_ ; do
  for div in $div_ ; do
    echo "----------------------------------------------------------------"
    echo "Now running the experiments with latency div =" $resp $div
    echo "----------------------------------------------------------------"

    if [[ $generator == "table" ]];
    then
      # Generate tables for parameter sweeps
      if [[ $resp == "106W85R" ]];
      then
        for lat in $lats_106_85_ ; do
          limeapps=$PWD
          cd $LIME/ip/2018.2/llnl.gov_user_axi_delayv_1.0.0/hdl/gaussian_delay
          python gaussian_delay.py $lat $div
          cd $limeapps
        done
        arr=($lats_106_85_)
      else
        for lat in $lats_400_200_ ; do
          limeapps=$PWD
          cd $LIME/ip/2018.2/llnl.gov_user_axi_delayv_1.0.0/hdl/gaussian_delay
          python gaussian_delay.py $lat $div
          cd $limeapps
        done
        arr=($lats_400_200_)
      fi

      # Copy the tables to the LIME code
      cp $LIME/shared/gdt_data_g${arr[0]}_mu_div$div.txt $LIME/shared/gdt_data_cpu_sram_write.txt
      cp $LIME/shared/gdt_data_g${arr[1]}_mu_div$div.txt $LIME/shared/gdt_data_cpu_sram_read.txt
      cp $LIME/shared/gdt_data_g${arr[2]}_mu_div$div.txt $LIME/shared/gdt_data_cpu_dram_write.txt
      cp $LIME/shared/gdt_data_g${arr[3]}_mu_div$div.txt $LIME/shared/gdt_data_cpu_dram_read.txt
      cp $LIME/shared/gdt_data_g${arr[4]}_mu_div$div.txt $LIME/shared/gdt_data_acc_sram_write.txt
      cp $LIME/shared/gdt_data_g${arr[5]}_mu_div$div.txt $LIME/shared/gdt_data_acc_sram_read.txt
      cp $LIME/shared/gdt_data_g${arr[6]}_mu_div$div.txt $LIME/shared/gdt_data_acc_dram_write.txt
      cp $LIME/shared/gdt_data_g${arr[7]}_mu_div$div.txt $LIME/shared/gdt_data_acc_dram_read.txt

      var_delay="_GDT_"

    else

      var_delay="_PWCLT"$resp"_,STD=_MUDIVBY"$div"_" 

    fi

    # Do benchmark runs
    # cd image/zup
    # make clean
    # ds_="4 8 16 32 64"
    # for d in $ds_ ; do
    #   make D=CLOCKS,STATS,VAR_DELAY=$var_delay RUN_ARGS="-d$d -v15 -w24000 -h16000 pat pat" run
    # done
    # for d in $ds_ ; do
    #   make D=CLIENT,CLOCKS,STATS,VAR_DELAY=$var_delay RUN_ARGS="-d$d -v15 -w16000 -h8000 pat pat" run
    # done
    # cd ../..

    # cd pager/zup 
    # make clean
    # make D=STATS,CLOCKS,VAR_DELAY=$var_delay run
    # make D=STATS,CLOCKS,CLIENT,VAR_DELAY=$var_delay run
    # cd ../..

    # cd spmv/zup
    # make clean
    # make D=CLOCKS,STATS,VAR_DELAY=$var_delay run
    # make D=CLOCKS,STATS,CLIENT,VAR_DELAY=$var_delay run
    # cd ../..

    cd randa/zup 
    make clean
    # make D=STATS,CLOCKS,VAR_DELAY=$var_delay run
    make D=STATS,CLOCKS,CLIENT,VAR_DELAY=$var_delay run
    cd ../..

    # cd rtb/zup 
    # make clean
    # make D=CLOCKS,STATS,USE_HASH,USE_OCM,VAR_DELAY=$var_delay RUN_ARGS="-e32Mi -l.60 -c -w1Mi -h.90 -z.99" run
    # make D=CLOCKS,STATS,USE_HASH,USE_OCM,OFFLOAD,VAR_DELAY=$var_delay RUN_ARGS="-e32Mi -l.60 -c -w1Mi -h.90 -z.99" run
    # cd ../..

    # cd strm/zup
    # make clean
    # make D=CLOCKS,STATS,VAR_DELAY=$var_delay run
    # cd ../..

    # cd xsb/zup
    # make clean
    # make D=CLOCKS,STATS,VAR_DELAY=$var_delay run
    # cd ../..
  done
done

# Kill the background serial port process
pkill screen

# Extract the runtimes from serial port log (from screenlog.0) and print to file
source misc/extract_runtimes.sh screenlog.0 > runtimes_final.csv

# Read the runtime results, generate the barchart and plot to file
octave misc/plots.m
