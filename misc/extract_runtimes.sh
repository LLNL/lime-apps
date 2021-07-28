# Extract the runtimes from serial port log and print to screen
awk '{for(i=1;i<=NF;i++)if($i=="overall" &&  $(i+1)=="time:")print $(i+2)}' screenlog.0
awk '{for(i=1;i<=NF;i++)if($i=="page" &&  $(i+1)=="rank")print $(i+2)}' screenlog.0 | cut -c 6-
awk '{for(i=1;i<=NF;i++)if($i=="SpMV" &&  $(i+1)=="time:")print $(i+2)}'  screenlog.0
awk '{for(i=1;i<=NF;i++)if($i=="time" &&  $(i+1)=="used" && $(i+2)=="=")print $(i+3)}' screenlog.0
awk '{for(i=1;i<=NF;i++)if($i=="Run" &&  $(i+1)=="time:")print $(i+2)}'  screenlog.0
awk '{for(i=1;i<=NF;i++)if($i=="Copy:")print $(i+2)}'  screenlog.0
awk '{for(i=1;i<=NF;i++)if($i=="Scale:")print $(i+2)}'  screenlog.0
awk '{for(i=1;i<=NF;i++)if($i=="Add:")print $(i+2)}'  screenlog.0
awk '{for(i=1;i<=NF;i++)if($i=="Triad:")print $(i+2)}'  screenlog.0
awk '{for(i=1;i<=NF;i++)if($i=="Runtime:")print $(i+1)}'  screenlog.0