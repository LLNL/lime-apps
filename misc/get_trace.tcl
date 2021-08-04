puts [connect]
puts [targets -set -filter {name =~"APU*"}]
after 2000
puts [target 11]
after 2000
puts [mrd -bin -file trace.bin traceaddr 0x01000000]
# change 0x01000000 to tracelength for full trace
after 4000
puts [disconnect]
puts [catch { disconnect };list]
puts [exit]
