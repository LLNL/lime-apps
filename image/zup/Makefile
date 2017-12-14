#
# $Id: $
#
# Description: Makefile for image program
#
# $Log: $
#

TARGET = image
VERSION = 1.0
LABEL = V$(subst .,_,$(VERSION))
EXE = .elf

BSP = $(WORKSPACE_LOC)/standalone_bsp_a53
APPS_DIR ?= $(WORKSPACE_LOC)/apps

#DEFS += -DVERSION=$(VERSION)
DEFS += -DZYNQ -DXILTIME -DSTANDALONE -DENTIRE

# ARM Processor
CC = aarch64-none-elf-gcc
CXX = aarch64-none-elf-g++
SIZE = aarch64-none-elf-size

SRC = $(APPS_DIR)/shared $(APPS_DIR)/image/src
HEADERS = ColorImage.hpp config.h alloc.h cache.h monitor.h ticks.h clocks.h
MODULES += ColorImage

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

ifneq (,$(wildcard ../src/Decimate2D.hpp))
  HEADERS += Decimate2D.hpp
endif

ifneq (,$(findstring CLIENT,$(DEFS)))
  DEFS += -DUSE_SP -DUSE_OCM
  HEADERS += lsu_cmd.h aport.h stream.h
  MODULES += lsu_cmd aport stream
endif

OBJECTS = $(addsuffix .o,$(TARGET) $(MODULES))

VPATH = $(subst ' ',:,$(SRC))

OPT = -O3 -ffast-math
# -funsafe-math-optimizations
# Zynq UltraScale+ MPSoC: ARMv8-A architecture, Cortex-A53 processor, VFPv4 floating point, NEON co-processor, Crypto
# MACH = -march=armv8-a+simd+crypto -mcpu=cortex-a53 -mfpu=crypto-neon-fp-armv8 -mvectorize-with-neon-quad -mfloat-abi=hard
# -mneon-for-64bits (enable scalar 64-bit operations, gcc v4.9.x)
CPPFLAGS = $(DEFS)
CPPFLAGS += $(patsubst %,-I%,$(SRC))
CPPFLAGS += -I$(BSP)/psu_cortexa53_0/include
CFLAGS = $(MACH) $(OPT) -Wall
CXXFLAGS = $(CFLAGS)
LDFLAGS += -Wl,-T -Wl,cpu_lscript.ld -L$(BSP)/psu_cortexa53_0/lib
LDLIBS = -Wl,--start-group,-lxilffs,-lxil,-lgcc,-lc,--end-group

#VAR = $(if $(findstring CYGWIN,$(shell uname -s)),CYG_TRUE,CYG_FALSE)

.PHONY: all
all: $(TARGET)$(EXE)
ifneq (,$(and $(findstring CLIENT,$(DEFS)),$(wildcard ../mcu)))
	cd ../mcu && $(MAKE) D=
endif

.PHONY: check
check: $(TARGET)$(EXE)
	./$(TARGET)$(EXE) -d16 -v15 -w16000 -h8000 pat pat

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
ColorImage.o: ColorImage.hpp
lsu_cmd.o: lsu_cmd.h aport.h stream.h
aport.o: aport.h stream.h
stream.o: stream.h
$(M5OP).o: m5op.h m5ops.h
