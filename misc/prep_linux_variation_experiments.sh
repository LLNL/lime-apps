#This script prepares elf binaries and copies to the boot partition of the SD card to run under linux 

# Parameters to sweep
# div_="4 8 16 32"
div_="ivy"
# resps_="106W85R 400W200R"
resps_="106W85R"

# Table or PwCLT based generators.
# generators_="pwclt table"
generators_="table"
nongaussian="true" # Only if table is selected instead of Gaussian
experiment="strm"

# Individual channel latency clock cycles (mean of Gaussians)
# lats_106_85_="216 216 636 510 72 72 492 366"
# lats_106_85_="453 453 453 453 453 453 453 453"
# lats_106_85_="246 246 246 246 246 246 246 246"
lats_106_85_="1119 1119 1119 1119 1119 1119 1119 1119"

lats_400_200_="216 216 2400 1200 72 72 2256 1056"

# This is the FPGA serial port device to listen to
sdcard="/cygdrive/d"

# Sweep through the parameters
for gen in $generators_ ; do
  for resp in $resps_ ; do
    for div in $div_ ; do
      echo "----------------------------------------------------------------"
      echo "Now preparing the experiments with generator latency div =" $gen $resp $div
      echo "----------------------------------------------------------------"

      if [[ $gen == "table" ]];
      then
                
                # Generate tables for parameter sweeps
                if [[ $resp == "106W85R" ]];
                then
                    for lat in $lats_106_85_ ; do
                    limeapps=$PWD
                    cd /lime2/lime/ip/2018.2/llnl.gov_user_axi_delayv_1.0.0/hdl/gaussian_delay

                    # Use a non-gaussian distribution to populate the tables
                    if [[ $nongaussian == "true" ]];
                    then
                        python ivy_custom_delay.py $experiment
                    else
                        python gaussian_delay.py $lat $div
                    fi

                    cd $limeapps
                    done
                    arr=($lats_106_85_)
                else
                    for lat in $lats_400_200_ ; do
                    limeapps=$PWD
                    cd /lime2/lime/ip/2018.2/llnl.gov_user_axi_delayv_1.0.0/hdl/gaussian_delay
                    python gaussian_delay.py $lat $div
                    cd $limeapps
                    done
                    arr=($lats_400_200_)
                fi

                #  Copy the tables to the LIME code
                if [[ $nongaussian == "true" ]];
                then
                    cp /lime2/lime/shared/${experiment}_dt_data_custom.txt /lime2/lime/shared/gdt_data_cpu_sram_write.txt
                    cp /lime2/lime/shared/${experiment}_dt_data_custom.txt /lime2/lime/shared/gdt_data_cpu_sram_read.txt
                    cp /lime2/lime/shared/${experiment}_dt_data_custom.txt /lime2/lime/shared/gdt_data_cpu_dram_write.txt
                    cp /lime2/lime/shared/${experiment}_dt_data_custom.txt /lime2/lime/shared/gdt_data_cpu_dram_read.txt
                    cp /lime2/lime/shared/${experiment}_dt_data_custom.txt /lime2/lime/shared/gdt_data_acc_sram_write.txt
                    cp /lime2/lime/shared/${experiment}_dt_data_custom.txt /lime2/lime/shared/gdt_data_acc_sram_read.txt
                    cp /lime2/lime/shared/${experiment}_dt_data_custom.txt /lime2/lime/shared/gdt_data_acc_dram_write.txt
                    cp /lime2/lime/shared/${experiment}_dt_data_custom.txt /lime2/lime/shared/gdt_data_acc_dram_read.txt
                else
                    cp /lime2/lime/shared/gdt_data_g${arr[0]}_mu_div$div.txt /lime2/lime/shared/gdt_data_cpu_sram_write.txt
                    cp /lime2/lime/shared/gdt_data_g${arr[1]}_mu_div$div.txt /lime2/lime/shared/gdt_data_cpu_sram_read.txt
                    cp /lime2/lime/shared/gdt_data_g${arr[2]}_mu_div$div.txt /lime2/lime/shared/gdt_data_cpu_dram_write.txt
                    cp /lime2/lime/shared/gdt_data_g${arr[3]}_mu_div$div.txt /lime2/lime/shared/gdt_data_cpu_dram_read.txt
                    cp /lime2/lime/shared/gdt_data_g${arr[4]}_mu_div$div.txt /lime2/lime/shared/gdt_data_acc_sram_write.txt
                    cp /lime2/lime/shared/gdt_data_g${arr[5]}_mu_div$div.txt /lime2/lime/shared/gdt_data_acc_sram_read.txt
                    cp /lime2/lime/shared/gdt_data_g${arr[6]}_mu_div$div.txt /lime2/lime/shared/gdt_data_acc_dram_write.txt
                    cp /lime2/lime/shared/gdt_data_g${arr[7]}_mu_div$div.txt /lime2/lime/shared/gdt_data_acc_dram_read.txt
                fi

        var_delay="_GDT_"

      else

        var_delay="_PWCLT"$resp"_,STD=_MUDIVBY"$div"_" 

      fi

      # Do benchmark runs
      # cd image/arm_64
      # make clean
      # # make D=CLOCKS,STATS,VAR_DELAY=$var_delay && cp image.elf $sdcard/image_$gen$resp$div.elf
      # make D=CLOCKS,STATS,VAR_DELAY=$var_delay && cp image.elf $sdcard/image_${gen}_ivy.elf
      # cd ../..

      # cd pager/arm_64 
      # make clean
      # make D=STATS,CLOCKS,VAR_DELAY=$var_delay && cp pager.elf $sdcard/pager_$gen$resp$div.elf
      # cd ../..

      # cd spmv/arm_64
      # make clean
      # make D=CLOCKS,STATS,VAR_DELAY=$var_delay && cp spmv.elf $sdcard/spmv_$gen$resp$div.elf
      # cd ../..

      # cd randa/arm_64 
      # make clean
      # # make D=STATS,CLOCKS,VAR_DELAY=$var_delay && cp randa.elf $sdcard/randa_$gen$resp$div.elf
      # make D=STATS,CLOCKS,VAR_DELAY=$var_delay && cp randa.elf $sdcard/randa_${gen}_ivy.elf
      # cd ../..

      # cd rtb/arm_64 
      # make clean
      # make D=CLOCKS,STATS,USE_HASH,USE_OCM,VAR_DELAY=$var_delay && cp rtb.elf $sdcard/rtb_$gen$resp$div.elf
      # cd ../..

      cd strm/arm_64
      make clean
      # make D=CLOCKS,STATS,VAR_DELAY=$var_delay && cp strm.elf $sdcard/strm_$gen$resp$div.elf
      make D=CLOCKS,STATS,VAR_DELAY=$var_delay && cp strm.elf $sdcard/strm_${gen}_ivy.elf
      cd ../..

      # cd xsb/arm_64
      # make clean
      # make D=CLOCKS,STATS,VAR_DELAY=$var_delay && cp xsb.elf $sdcard/xsb_$gen$resp$div.elf
      # cd ../..
    done
  done
done

cp misc/run_linux_variation_experiments.sh $sdcard/run_linux_variation_experiments.sh 
