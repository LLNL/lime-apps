#
# $Id: $
#
# Description: Makefile for pager program
#
# $Log: $
#

TARGET = pager
VERSION = 1.0
LABEL = V$(subst .,_,$(VERSION))

#DEFS += -DVERSION=$(VERSION)
#DEFS += -DTIMEOFDAY

SRC = ../../shared ../src
HEADERS = config.h alloc.h cache.h monitor.h IndexArray.hpp hash_nbits.hpp ticks.h clocks.h
#MODULES += 

# comma separated list of defines
ifdef D
  SEP := ,
  DEFS += $(patsubst %,-D%,$(subst $(SEP), ,$(D)))
endif

ifneq (,$(findstring M5,$(DEFS)))
  M5OP = m5op_x86
  SRC += ../../m5
  MODULES += $(M5OP)
endif

OBJECTS = $(addsuffix .o,$(TARGET) $(MODULES))

VPATH = $(subst ' ',:,$(SRC))

OPT = -O3
#OPT += -ftree-vectorize -ffast-math
#MACH = -march=core2
CPPFLAGS = $(DEFS)
CPPFLAGS += $(patsubst %,-I%,$(SRC))
ifdef BOOST_ROOT
  CPPFLAGS += -I$(BOOST_ROOT)
else ifneq (,$(wildcard $(HOME)/local/include/boost))
  CPPFLAGS += -I$(HOME)/local/include
endif
CFLAGS = $(MACH) $(OPT) -Wall
CXXFLAGS = $(CFLAGS)
LDFLAGS += -static
#LDLIBS = -lrt

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
$(TARGET).o: $(HEADERS)
$(M5OP).o: m5op.h m5ops.h
