#
# $Id: $
#
# Description: Makefile for pager_mcu program
#
# $Log: $
#

TARGET = pager_mcu
VERSION = 1.0
LABEL = V$(subst .,_,$(VERSION))
EXE = .elf

BSP = $(WORKSPACE_LOC)/standalone_bsp_mb
APPS_DIR ?= $(WORKSPACE_LOC)/apps

#DEFS += -DVERSION=$(VERSION)

# Xilinx MicroBlaze Soft Processor
CC = mb-gcc
CXX = mb-g++
SIZE = mb-size

SRC = $(APPS_DIR)/shared $(APPS_DIR)/pager/src
HEADERS = IndexArray.hpp lsu_cmd.h aport.h stream.h
MODULES += mcu_main lsu_cmd aport stream

# comma separated list of defines
ifdef D
  SEP := ,
  DEFS += $(patsubst %,-D%,$(subst $(SEP), ,$(D)))
endif

OBJECTS = $(addsuffix .o,$(MODULES))

VPATH = $(subst ' ',:,$(SRC))

OPTM = -O2
MACH = -mcpu=v9.6 -mlittle-endian -mno-xl-reorder -mxl-barrel-shift -mno-xl-soft-div -mno-xl-soft-mul
CPPFLAGS = $(DEFS)
CPPFLAGS += $(patsubst %,-I%,$(SRC))
CPPFLAGS += -I$(BSP)/engine_0_mcu_0_microblaze_0/include
CFLAGS = $(MACH) $(OPTM) -Wall -fno-strict-aliasing -ffunction-sections -fdata-sections
CXXFLAGS = $(CFLAGS)
LDFLAGS += -Wl,-T -Wl,mcu_lscript.ld -L$(BSP)/engine_0_mcu_0_microblaze_0/lib -Wl,--no-relax -Wl,--gc-sections
LDLIBS = -Wl,--start-group,-lxil,-lgcc,-lc,--end-group

#VAR = $(if $(findstring CYGWIN,$(shell uname -s)),CYG_TRUE,CYG_FALSE)

.PHONY: all
all: $(TARGET)$(EXE)

.PHONY: check
check: $(TARGET)$(EXE)
	./$(TARGET)$(EXE)

.PHONY: clean
clean:
	$(RM) $(wildcard *.o) $(TARGET)$(EXE) $(TARGET)$(EXE).size

.PHONY: vars
vars:
	@echo TARGET: $(TARGET)
	@echo VERSION: $(VERSION)
	@echo LABEL: $(LABEL)
	@echo OBJECTS: $(OBJECTS)
	@echo DEFS: $(DEFS)

$(TARGET)$(EXE): $(OBJECTS) mcu_lscript.ld
	$(LINK.cpp) $(OBJECTS) $(LOADLIBES) $(LDLIBS) -o $@
	$(SIZE) $@  |tee $@.size

$(OBJECTS): $(MAKEFILE_LIST) # rebuild if MAKEFILE changes
# establish module specific dependencies
# module.o: module.h
mcu_main.o: $(HEADERS)
lsu_cmd.o: lsu_cmd.h stream.h
aport.o: aport.h stream.h
stream.o: stream.h
