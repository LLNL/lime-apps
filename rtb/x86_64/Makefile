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

#DEFS += -DVERSION=$(VERSION)
DEFS += -DTIMEOFDAY

SRC = ../../shared ../src
MODULES = fasta path
HEADERS = config.h alloc.h cache.h monitor.h ticks.h clocks.h
HEADERS += $(addsuffix .h,$(MODULES))

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

ifneq (,$(wildcard ../src/KVstore.hpp))
  HEADERS += KVstore.hpp short.h
  MODULES += short
endif

OBJECTS = $(addsuffix .o,$(TARGET) $(MODULES))

VPATH = $(subst ' ',:,$(SRC))

OPT = -O3
#OPT += -ftree-vectorize -ffast-math
#MACH = -march=core2
CPPFLAGS = $(DEFS)
CPPFLAGS += $(patsubst %,-I%,$(SRC))
CFLAGS = $(MACH) $(OPT) -Wall
CXXFLAGS = $(CFLAGS) -std=c++11
LDFLAGS += -static
#LDLIBS = -lrt

.PHONY: all
all: $(TARGET)

.PHONY: check
check: $(TARGET)
	./$(TARGET) -e1K -k16 -r../src/testdb.fa -q../src/testqr.fa

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
fasta.o: fasta.h
path.o: path.h
short.o: short.h
$(M5OP).o: m5op.h m5ops.h
