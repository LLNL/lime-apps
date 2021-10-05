make build=zup fpga
sleep 15

# Parameters to sweep
# div_="4 8 16 32"
div_="4"
# resps_="106W85R 400W200R"
resps_="400W200R"
# generator_="pwclt table"
generator_="pwclt table"
# experiment="image"
experiment="randa"

# This is the FPGA serial port device to listen to
serialport="/dev/ttyS3"

# Individual channel latency clock cycles (mean of Gaussians)
lats_106_85_="216 216 636 510 72 72 492 366"
lats_400_200_="216 216 2400 1200 72 72 2256 1056"

# Sweep through the parameters
for generator in $generator_ ; do
    for resp in $resps_ ; do
        for div in $div_ ; do

            echo "----------------------------------------------------------------"
            echo "Now running the experiments with latency div =" $generator $resp $div
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
                cp $LIME/shared/gdt_data/gdt_data_g${arr[0]}_mu_div$div.txt $LIME/shared/gdt_data/gdt_data_cpu_sram_write.txt
                cp $LIME/shared/gdt_data/gdt_data_g${arr[1]}_mu_div$div.txt $LIME/shared/gdt_data/gdt_data_cpu_sram_read.txt
                cp $LIME/shared/gdt_data/gdt_data_g${arr[2]}_mu_div$div.txt $LIME/shared/gdt_data/gdt_data_cpu_dram_write.txt
                cp $LIME/shared/gdt_data/gdt_data_g${arr[3]}_mu_div$div.txt $LIME/shared/gdt_data/gdt_data_cpu_dram_read.txt
                cp $LIME/shared/gdt_data/gdt_data_g${arr[4]}_mu_div$div.txt $LIME/shared/gdt_data/gdt_data_acc_sram_write.txt
                cp $LIME/shared/gdt_data/gdt_data_g${arr[5]}_mu_div$div.txt $LIME/shared/gdt_data/gdt_data_acc_sram_read.txt
                cp $LIME/shared/gdt_data/gdt_data_g${arr[6]}_mu_div$div.txt $LIME/shared/gdt_data/gdt_data_acc_dram_write.txt
                cp $LIME/shared/gdt_data/gdt_data_g${arr[7]}_mu_div$div.txt $LIME/shared/gdt_data/gdt_data_acc_dram_read.txt

                var_delay="_GDT_"

            else

                var_delay="_PWCLT"$resp"_,STD=_MUDIVBY"$div"_" 

            fi

            # Clear the earlier serial port log & kill bg screen process if exists and 
            pkill screen
            rm -rf screenlog.0
            sleep 5

            echo "Old serial port log cleared"

            # Start listening to the serial port in background
            screen -dm -L $serialport 115200

            echo "Started listening to the serial port"
            sleep 3

            cd $experiment/zup
            make clean
            if [[ $experiment == "image" || $experiment == "spmv" || $experiment == "randa" ]];
            then
                make D=CLOCKS,STATS,TRACE=_TALL_,CLIENT,VAR_DELAY=$var_delay run > /dev/null 2>&1 & 
            elif [[ $experiment == "rtb" ]];
            then
                make D=CLOCKS,STATS,TRACE=_TALL_,USE_HASH,USE_OCM,OFFLOAD,USE_SD,VAR_DELAY=$var_delay RUN_ARGS="-e32Mi -l.60 -c -w1Mi -h.90 -z.99" run > /dev/null 2>&1 & 
            else
                echo "ERROR: The experiment is not recognized!"
                exit 0
            fi
            cd ../..

            echo "The application is running. Wait until the trace is available. "
            while : ; do
                sleep 3
                isInFile=$(cat screenlog.0 | grep -c "trace")
                if [ $isInFile -eq 0 ]; then
                    continue
                else
                    break
                fi
            done
            
            pkill make 
            pkill screen

            # Extract trace address and length
            traceaddr=`awk '{for(i=1;i<=NF;i++)if($i=="address:")print $(i+1)}'  screenlog.0 | cut -d "," -f1`
            tracelength=`awk '{for(i=1;i<=NF;i++)if($i=="address:")print $(i+3)}'  screenlog.0 | rev | cut -c 2- | rev`
            # tracelength=`printf '%d\n' $tracelength` # If decimal length is preferred
            echo "Parsed trace address: " $traceaddr " length: " $tracelength
            
            # Now substitute the trace length and address
            rm -rf misc/get_trace_customized.tcl
            sed "s/traceaddr/$traceaddr/g ; s/tracelength/$tracelength/g" misc/get_trace.tcl > misc/get_trace_customized.tcl

            echo "Now reading the trace..."
            rm -rf trace.bin
            xsdb.bat misc/get_trace_customized.tcl

            echo "Trace read done. Now generating CSV."

            $LIME/trace/parser.exe trace.bin $experiment$generator$resp$div.csv

            echo "CSV generated. Now plotting the histogram."

            limeapps=$PWD
            cd $LIME/trace/tools
            ./lplot.sh -a -b$experiment$generator$resp$div -f -s $limeapps/$experiment$generator$resp$div.csv $generator$resp$div   
            cd $limeapps     

            echo "Plot done."
        done
    done
done
