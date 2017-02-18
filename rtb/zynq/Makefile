#
# $Id: $
#
# Description: Makefile for rtb program
#
# $Log: $
#

TARGET = rtb
VERSION = 3.0
LABEL = V$(subst .,_,$(VERSION))
EXE = .elf

BSP = $(WORKSPACE_LOC)/standalone_bsp_a9
APPS_DIR ?= $(WORKSPACE_LOC)/apps

#DEFS += -DVERSION=$(VERSION)
DEFS += -DZYNQ -DXILTIME -DSTANDALONE

# ARM Processor
# CC = arm-xilinx-eabi-gcc
# CXX = arm-xilinx-eabi-g++
# SIZE = arm-xilinx-eabi-size
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-g++
SIZE = arm-none-eabi-size

SRC = $(APPS_DIR)/shared $(APPS_DIR)/rtb/src
MODULES = fasta path
HEADERS = config.h alloc.h cache.h monitor.h ticks.h clocks.h
HEADERS += $(addsuffix .h,$(MODULES))

# comma separated list of defines
ifdef D
  SEP := ,
  DEFS += $(patsubst %,-D%,$(subst $(SEP), ,$(D)))
endif

ifneq (,$(findstring M5,$(DEFS)))
  M5OP = m5op_arm
  SRC += $(APPS_DIR)/m5
  HEADERS += m5op.h
  MODULES += $(M5OP)
endif

ifneq (,$(wildcard ../src/KVstore.hpp))
  HEADERS += KVstore.hpp short.h
  MODULES += short
endif

ifneq (,$(or $(findstring CLIENT,$(DEFS)), $(findstring USE_LSU,$(DEFS))))
  # Must use scratch pad because code expects inner (L1) cacheable only
  DEFS += -DUSE_SP -DUSE_OCM
  HEADERS += stream.h lsu_cmd.h
  MODULES += stream lsu_cmd
endif

OBJECTS = $(addsuffix .o, fatfs $(TARGET) $(MODULES))

VPATH = $(subst ' ',:,$(SRC))

OPT = -O3 -ffast-math
# Zynq-7000 SoC: ARM v7-A architecture, Cortex-A9 processor, VFPv3 floating point, NEON co-processor
# MACH = -march=armv7-a -mcpu=cortex-a9 -mfpu=neon -mvectorize-with-neon-quad -mfloat-abi=softfp
MACH = -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard
# -mneon-for-64bits (gcc v4.9.x)
CPPFLAGS = $(DEFS)
CPPFLAGS += $(patsubst %,-I%,$(SRC))
CPPFLAGS += -I$(BSP)/ps7_cortexa9_0/include
CFLAGS = $(MACH) $(OPT) -Wall
CXXFLAGS = $(CFLAGS) -std=c++11
# LDFLAGS += -Wl,-T -Wl,cpu_lscript.ld -L$(BSP)/ps7_cortexa9_0/lib
LDFLAGS += -Wl,-build-id=none -specs=Xilinx.spec -Wl,-T -Wl,cpu_lscript.ld -L$(BSP)/ps7_cortexa9_0/lib
# LDFLAGS += -Wl,--gc-sections # cause the linker to prune unused code
LDLIBS = -Wl,--start-group,-lxilffs,-lxil,-lgcc,-lc,-lstdc++,--end-group

.PHONY: all
all: $(TARGET)$(EXE)
ifneq (,$(and $(findstring CLIENT,$(DEFS)),$(wildcard ../mcu)))
	cd ../mcu && $(MAKE) D=
endif

.PHONY: check
check: $(TARGET)$(EXE)
	./$(TARGET)$(EXE)

.PHONY: clean
clean:
	$(RM) $(wildcard *.o) $(TARGET)$(EXE) $(TARGET)$(EXE).size
ifneq (,$(wildcard ../mcu))
	cd ../mcu && $(MAKE) clean
endif

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
fasta.o: fasta.h
path.o: path.h
short.o: short.h
stream.o: stream.h
$(M5OP).o: m5op.h m5ops.h
