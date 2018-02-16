TARGET = dgemm
VERSION = 1.0
SRC = ../src ../../shared
DEFS += -DHPL_CALL_VSIPL
MODULES = dgemm HPL_dgemm HPL_dgemv HPL_dlamch HPL_dscal tstdgemm
RUN_ARGS = 
# CXXFLAGS += -std=c++11
# LDFLAGS += 
# LDLIBS += -lstdc++
