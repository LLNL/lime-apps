#
# $Id: $
#
# Description: Makefile for fasta program
#
# $Log: $
#

TARGET = fasta
VERSION = 1.5
LABEL = V$(subst .,_,$(VERSION))

#DEFS += -DVERSION=$(VERSION)
#DEFS += -DTIMEOFDAY
DEFS += -DFASTA_TEST

SRC = ../src
#MODULES = 
HEADERS = fasta.h
#HEADERS += $(addsuffix .h,$(MODULES))

# comma separated list of defines
ifdef D
  SEP := ,
  DEFS += $(patsubst %,-D%,$(subst $(SEP), ,$(D)))
endif

OBJECTS = $(addsuffix .o,$(TARGET) $(MODULES))

VPATH = $(subst ' ',:,$(SRC))

OPT = -O3
#OPT += -ftree-vectorize -ffast-math
#MACH = -march=core2
CPPFLAGS = $(DEFS)
CPPFLAGS += $(patsubst %,-I%,$(SRC))
CFLAGS = $(MACH) $(OPT) -Wall
#CXXFLAGS = $(CFLAGS) -std=c++11
LDFLAGS += -static
#LDLIBS = -lrt

.PHONY: all
all: $(TARGET)

.PHONY: check
check: $(TARGET)
	./$(TARGET) -s ../src/testqr.fa -

.PHONY: clean
clean:
	$(RM) $(OBJECTS) $(TARGET)$(EXE)

.PHONY: vars
vars:
	@echo TARGET: $(TARGET)
	@echo VERSION: $(VERSION)
	@echo LABEL: $(LABEL)
	@echo OBJECTS: $(OBJECTS)
	@echo DEFS: $(DEFS)

$(TARGET): $(OBJECTS)

$(OBJECTS): $(MAKEFILE_LIST) # rebuild if MAKEFILE changes
# establish module specific dependencies
#module.o: module.h
$(TARGET).o: $(HEADERS)
