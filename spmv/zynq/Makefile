#
# $Id: $
#
# Description: Makefile for spmv program
#
# $Log: $
#

TARGET = spmv
VERSION = 1.0
LABEL = V$(subst .,_,$(VERSION))
EXE = .elf

BSP = $(WORKSPACE_LOC)/standalone_bsp_a9
APPS_DIR ?= $(WORKSPACE_LOC)/apps

#DEFS += -DVERSION=$(VERSION)
DEFS += -DZYNQ -DXILTIME -DSTANDALONE

# ARM Processor
CC = arm-xilinx-eabi-gcc
CXX = arm-xilinx-eabi-g++
SIZE = arm-xilinx-eabi-size

SRC = $(addprefix $(APPS_DIR)/,shared spmv/src $(addprefix spmv/src/,bebop_util matrix_generator spmvbench))
HEADERS = config.h alloc.h cache.h monitor.h IndexArray.hpp ticks.h clocks.h
MODULES += block_smvm_code smvm_benchmark smvm_timing_results smvm_timing_run smvm_verify_result timing
MODULES += bcoo_matrix bcsr_matrix create_rand
MODULES += __complex mt19937ar random_number smvm_malloc smvm_util sort_joint_arrays

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

OPT = -O3 -ftree-vectorize -ffast-math
MACH = -mcpu=cortex-a9 -mfpu=neon -mvectorize-with-neon-quad -mfloat-abi=softfp
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
ifneq (,$(and $(findstring CLIENT,$(DEFS)),$(wildcard ../mcu)))
	cd ../mcu && $(MAKE) D=
endif

.PHONY: check
check: $(TARGET)$(EXE)
	./$(TARGET)$(EXE) -s18 -v15

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
$(TARGET).o: smvm_benchmark.h smvm_timing_results.h $(HEADERS)
stream.o: stream.h
$(M5OP).o: m5op.h m5ops.h

# bebop_util
__complex.o: __complex.h
random_number.o: random_number.h smvm_malloc.h smvm_util.h
smvm_malloc.o: smvm_malloc.h smvm_util.h
smvm_util.o: smvm_util.h smvm_malloc.h random_number.h
sort_joint_arrays.o: sort_joint_arrays.h random_number.h smvm_malloc.h smvm_util.h

# matrix_generator
bcoo_matrix.o: bcoo_matrix.h bcsr_matrix.h enumerations.h __complex.h random_number.h smvm_malloc.h smvm_util.h sort_joint_arrays.h
bcsr_matrix.o: bcsr_matrix.h enumerations.h __complex.h smvm_malloc.h smvm_util.h
create_rand.o: create_rand.h bcoo_matrix.h bcsr_matrix.h enumerations.h random_number.h smvm_malloc.h smvm_util.h sort_joint_arrays.h

# spmvbench
block_smvm_code.o: block_smvm_code.h $(HEADERS)
smvm_benchmark.o: smvm_benchmark.h smvm_timing_run.h smvm_timing_results.h smvm_verify_result.h smvm_util.h smvm_malloc.h create_rand.h bcsr_matrix.h
smvm_timing_results.o: smvm_timing_results.h smvm_util.h
smvm_timing_run.o: smvm_timing_run.h smvm_timing_results.h smvm_verify_result.h timing.h smvm_util.h smvm_malloc.h block_smvm_code.h $(HEADERS)
smvm_verify_result.o: smvm_verify_result.h smvm_malloc.h smvm_util.h
timing.o: timing.h
