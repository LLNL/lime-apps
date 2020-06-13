TARGET = dgemm
VERSION = 1.0
SRC += ../src $(SHARED)
DEFS += -DHPL_CALL_VSIPL
MODULES += dgemm HPL_dgemm HPL_dgemv HPL_dlamch HPL_dscal tstdgemm
# OPT = -O2 -ffloat-store -frounding-math
ifeq ($(ARG),1)
  RUN_ARGS = -s24
else
  RUN_ARGS = -s20
endif
# CXXFLAGS += -std=c++11
# LDFLAGS += 
# LDLIBS += -lstdc++
