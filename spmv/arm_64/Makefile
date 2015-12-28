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

#DEFS += -DVERSION=$(VERSION)
#DEFS += -DTIMEOFDAY
#DEFS += -DPOSIX_GETTIMEOFDAY
DEFS += -DHAVE__COMPLEX

SRC = ../../shared ../src ../src/bebop_util ../src/matrix_generator ../src/spmvbench
HEADERS = config.h alloc.h cache.h monitor.h ticks.h clocks.h
MODULES += block_smvm_code smvm_benchmark smvm_timing_results smvm_timing_run smvm_verify_result timing
MODULES += bcoo_matrix bcsr_matrix create_rand
MODULES += __complex mt19937ar random_number smvm_malloc smvm_util sort_joint_arrays

# comma separated list of defines
ifdef D
  SEP := ,
  DEFS += $(patsubst %,-D%,$(subst $(SEP), ,$(D)))
endif

ifneq (,$(findstring M5,$(DEFS)))
  M5OP = m5op_arm_A64
  SRC += ../../m5
  MODULES += $(M5OP)
endif

ifneq (,$(wildcard ../src/IndexArray.hpp))
  HEADERS += IndexArray.hpp
endif

OBJECTS = $(addsuffix .o,$(TARGET) $(MODULES))

VPATH = $(subst ' ',:,$(SRC))

OPT = -O3
#OPT += -ftree-vectorize -ffast-math
MACH = -march=armv8-a
CPPFLAGS = $(DEFS)
CPPFLAGS += $(patsubst %,-I%,$(SRC))
CFLAGS = $(MACH) $(OPT) -Wall
CXXFLAGS = $(CFLAGS)
LDFLAGS += -static
LDLIBS = -L$(HOME)/local/lib
LDLIBS += -lm

#VAR = $(if $(findstring CYGWIN,$(shell uname -s)),CYG_TRUE,CYG_FALSE)

.PHONY: all
all: $(TARGET)

.PHONY: check
check: $(TARGET)
	./$(TARGET) -s18 -v15

.PHONY: clean
clean:
	$(RM) $(wildcard *.o) $(TARGET)$(EXE)

.PHONY: vars
vars:
	@echo TARGET: $(TARGET)
	@echo VERSION: $(VERSION)
	@echo LABEL: $(LABEL)
	@echo OBJECTS: $(OBJECTS)
	@echo DEFS: $(DEFS)

$(TARGET): $(OBJECTS)
	$(LINK.cpp) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(OBJECTS): $(MAKEFILE_LIST) # rebuild if MAKEFILE changes
# establish module specific dependencies
# module.o: module.h
$(TARGET).o: smvm_benchmark.h smvm_timing_results.h $(HEADERS)
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
