#
# $Id: $
#
# Description: Makefile for dgemm program
#
# $Log: $
#

TARGET = dgemm
VERSION = 1.0
LABEL = V$(subst .,_,$(VERSION))

#DEFS += -DVERSION=$(VERSION)
#DEFS += -DTIMEOFDAY
DEFS += -DHPL_CALL_VSIPL

SRC = ../../shared ../src
HEADERS = hpcc.h config.h alloc.h cache.h monitor.h ticks.h clocks.h
MODULES += HPL_dgemm HPL_dgemv HPL_dlamch HPL_dscal tstdgemm

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

OBJECTS = $(addsuffix .o,$(TARGET) $(MODULES))

VPATH = $(subst ' ',:,$(SRC))

OPT = -O3
MACH = -march=armv8-a
CPPFLAGS = $(DEFS)
CPPFLAGS += $(patsubst %,-I%,$(SRC))
CFLAGS = $(MACH) $(OPT) -Wall
CXXFLAGS = $(CFLAGS)
LDFLAGS += -static
LDLIBS = -L$(HOME)/local/lib

#VAR = $(if $(findstring CYGWIN,$(shell uname -s)),CYG_TRUE,CYG_FALSE)

.PHONY: all
all: $(TARGET)

.PHONY: check
check: $(TARGET)
	./$(TARGET)

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
$(TARGET).o: $(HEADERS)
tstdgemm.o: HPL_misc.h HPL_blas.h HPL_auxil.h $(HEADERS)
HPL_dgemm.o: HPL_misc.h HPL_blas.h
HPL_dgemv.o: HPL_misc.h HPL_blas.h
HPL_dlamch.o: HPL_misc.h HPL_blas.h HPL_auxil.h
HPL_dscal.o: HPL_misc.h HPL_blas.h
$(M5OP).o: m5op.h m5ops.h
