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

#DEFS += -DVERSION=$(VERSION)
#DEFS += -DTIMEOFDAY
DEFS += -DSTREAM_ARRAY_SIZE=20000000
DEFS += -DNTIMES=2

SRC = ../../shared ../src
HEADERS = config.h alloc.h cache.h monitor.h ticks.h clocks.h
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
CFLAGS = $(MACH) $(OPT) -Wall
CXXFLAGS = $(CFLAGS)
LDFLAGS += -static
#LDLIBS = -lm
ifneq (,$(findstring GETTIME,$(DEFS)))
  LDLIBS += -lrt
endif

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
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(OBJECTS): $(MAKEFILE_LIST) # rebuild if MAKEFILE changes
# establish module specific dependencies
# module.o: module.h
$(TARGET).o: $(HEADERS)
$(M5OP).o: m5op.h m5ops.h
