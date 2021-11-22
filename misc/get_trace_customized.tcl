puts [connect]
puts [targets -set -filter {name =~"APU*"}]
after 2000
puts [target 11]
after 2000
puts [stop]
puts [mrd -bin -file trace.bin 0x30ED00 0x01000000]
# puts [mrd -bin -file trace.bin 0x30ED00 0x135A0040]
# change 0x01000000 to 0x135A0040 for full trace
after 4000
puts [disconnect]
puts [catch { disconnect };list]
puts [exit]
