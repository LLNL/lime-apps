#
# $Id: $
#
# Description: Makefile for strm program
#
# $Log: $
#

TARGET = strm
VERSION = 1.0
LABEL = V$(subst .,_,$(VERSION))
EXE = .elf

BSP = $(WORKSPACE_LOC)/standalone_bsp_a9
APPS_DIR ?= $(WORKSPACE_LOC)/apps

#DEFS += -DVERSION=$(VERSION)
DEFS += -DZYNQ -DXILTIME -DSTANDALONE
DEFS += -DSTREAM_ARRAY_SIZE=20000000
DEFS += -DNTIMES=2

# ARM Processor
CC = arm-xilinx-eabi-gcc
CXX = arm-xilinx-eabi-g++
SIZE = arm-xilinx-eabi-size

SRC = $(APPS_DIR)/shared $(APPS_DIR)/strm/src
HEADERS = config.h alloc.h cache.h monitor.h ticks.h clocks.h
#MODULES += 

# comma separated list of defines
ifdef D
  SEP := ,
  DEFS += $(patsubst %,-D%,$(subst $(SEP), ,$(D)))
endif

ifneq (,$(findstring M5,$(DEFS)))
  M5OP = m5op_arm
  SRC += $(APPS_DIR)/m5
  MODULES += $(M5OP)
endif

ifneq (,$(findstring CLIENT,$(DEFS)))
  # Must use scratch pad because code expects inner (L1) cacheable only
  DEFS += -DUSE_SP -DUSE_OCM
  HEADERS += stream.h
  MODULES += stream
endif

OBJECTS = $(addsuffix .o,$(TARGET) $(MODULES))

VPATH = $(subst ' ',:,$(SRC))

OPT = -O3 -ffast-math
# Zynq-7000 SoC: ARM v7-A architecture, Cortex-A9 processor, VFPv3 floating point, NEON co-processor
MACH = -march=armv7-a -mcpu=cortex-a9 -mfpu=neon -mvectorize-with-neon-quad -mfloat-abi=softfp
# -mneon-for-64bits (gcc v4.9.x)
CPPFLAGS = $(DEFS)
CPPFLAGS += $(patsubst %,-I%,$(SRC))
CPPFLAGS += -I$(BSP)/ps7_cortexa9_0/include
CFLAGS = $(MACH) $(OPT) -Wall
CXXFLAGS = $(CFLAGS)
LDFLAGS += -Wl,-T -Wl,cpu_lscript.ld -L$(BSP)/ps7_cortexa9_0/lib
LDLIBS = -Wl,--start-group,-lxilffs,-lxil,-lgcc,-lc,-lstdc++,--end-group

#VAR = $(if $(findstring CYGWIN,$(shell uname -s)),CYG_TRUE,CYG_FALSE)

.PHONY: all
all: $(TARGET)$(EXE)
#ifneq (,$(and $(findstring CLIENT,$(DEFS)),$(wildcard ../mcu)))
#	cd ../mcu && $(MAKE) D=
#endif

.PHONY: check
check: $(TARGET)$(EXE)
	./$(TARGET)$(EXE)

.PHONY: clean
clean:
	$(RM) $(wildcard *.o) $(TARGET)$(EXE) $(TARGET)$(EXE).size
#ifneq (,$(wildcard ../mcu))
#	cd ../mcu && $(MAKE) clean
#endif

.PHONY: vars
vars:
	@echo TARGET: $(TARGET)
	@echo VERSION: $(VERSION)
	@echo LABEL: $(LABEL)
	@echo OBJECTS: $(OBJECTS)
	@echo DEFS: $(DEFS)

$(TARGET)$(EXE): $(OBJECTS) cpu_lscript.ld
	$(LINK.cpp) $(OBJECTS) $(LOADLIBES) $(LDLIBS) -o $@
	$(SIZE) $@  |tee $@.size

$(OBJECTS): $(MAKEFILE_LIST) # rebuild if MAKEFILE changes
# establish module specific dependencies
# module.o: module.h
$(TARGET).o: $(HEADERS)
#stream.o: stream.h
$(M5OP).o: m5op.h m5ops.h
