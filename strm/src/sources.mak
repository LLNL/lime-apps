TARGET = strm
VERSION = 1.0
SRC = ../src ../../shared
MODULES = strm
DEFS += -DSTREAM_ARRAY_SIZE=20000000
DEFS += -DNTIMES=2
RUN_ARGS = 
# CXXFLAGS += -std=c++11
# LDFLAGS += 
# LDLIBS += -lstdc++
