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
BSP = $(WORKSPACE_LOC)/standalone_bsp_a9
EXE = .elf

#DEFS += -DVERSION=$(VERSION)
DEFS += -DZYNQ -DXILTIME -DSTANDALONE -DENTIRE

# ARM Processor
CC = arm-xilinx-eabi-gcc
CXX = arm-xilinx-eabi-g++
SIZE = arm-xilinx-eabi-size

SRC = .. ../../shared
HEADERS = config.h alloc.h cache.h monitor.h ColorImage.hpp Decimate2D.hpp ticks.h clocks.h
MODULES += ColorImage

# comma separated list of defines
ifdef D
  SEP := ,
  DEFS += $(patsubst %,-D%,$(subst $(SEP), ,$(D)))
endif

ifneq (,$(findstring M5,$(DEFS)))
  M5OP = m5op_arm
  SRC += ../../m5
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

OPT = -O3 -ftree-vectorize -ffast-math
MACH = -mcpu=cortex-a9 -mfpu=neon -mvectorize-with-neon-quad -mfloat-abi=softfp
CPPFLAGS = $(DEFS)
CPPFLAGS += $(patsubst %,-I%,$(SRC))
CPPFLAGS += -I$(BSP)/ps7_cortexa9_0/include
CFLAGS = $(MACH) $(OPT) -Wall
CXXFLAGS = $(CFLAGS)
LDFLAGS += -Wl,-T -Wl,cpu_lscript.ld -L$(BSP)/ps7_cortexa9_0/lib
LDLIBS = -Wl,--start-group,-lxilffs,-lxil,-lgcc,-lc,--end-group

#VAR = $(if $(findstring CYGWIN,$(shell uname -s)),CYG_TRUE,CYG_FALSE)

.PHONY: all
all: $(TARGET)$(EXE)
ifneq (,$(findstring CLIENT,$(DEFS)))
	cd mcu && $(MAKE) D=
endif

.PHONY: check
check: $(TARGET)$(EXE)
	./$(TARGET)$(EXE) -d16 -v15 -w16000 -h8000 pat pat

.PHONY: clean
clean:
	$(RM) $(OBJECTS) $(TARGET)$(EXE) $(TARGET)$(EXE).size
	cd mcu && $(MAKE) clean

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
stream.o: stream.h
$(M5OP).o: m5op.h m5ops.h
