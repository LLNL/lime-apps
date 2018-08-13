LABEL = V$(subst .,_,$(VERSION))
EXE = .elf

#DEFS += -DVERSION=$(VERSION)
DEFS += -DTIMEOFDAY

ifneq ($(findstring USE_MARGS,$(DEFS)),)
  DEFS += -DMARGS='"$(RUN_ARGS)"'
endif

ifneq ($(findstring M5,$(DEFS)),)
  SRC += ../../m5
  MODULES += m5op_arm_A64
endif

ifneq ($(findstring CLOCKS,$(DEFS)),)
  MODULES += clocks_ln
endif

ifneq ($(filter %STATS %TRACE,$(DEFS)),)
  MODULES += monitor_ln
endif

OBJECTS = $(addsuffix .o,$(MODULES))
VPATH = $(subst ' ',:,$(SRC))

# Linux libraries
CC = arm-linux-gnueabihf-gcc
CXX = arm-linux-gnueabihf-g++
SIZE = arm-linux-gnueabihf-size

OPT ?= -O3
ifdef OMP
  OPT += -fopenmp
endif
##MACH = -march=armv8-a    # targeting 32-bit zynq
CPPFLAGS += -MMD $(DEFS)
CPPFLAGS += $(patsubst %,-I%,$(SRC))
CFLAGS += $(MACH) $(OPT) -Wall
CXXFLAGS += $(CFLAGS)
LDFLAGS += -static -L$(HOME)/local/lib
#LDLIBS += -lrt

# Cancel version control implicit rules
%:: %,v
%:: RCS/%
%:: RCS/%,v
%:: s.%
%:: SCCS/s.%
# Delete default suffixes
.SUFFIXES:
# Define suffixes of interest
.SUFFIXES: .o .c .cc .cpp .h .hpp .d .mak

.PHONY: all
all: $(TARGET)$(EXE)

.PHONY: run
run: $(TARGET)$(EXE)
ifdef OMP
	OMP_NUM_THREADS=$(OMP) ./$(TARGET)$(EXE) $(RUN_ARGS)
else
	./$(TARGET)$(EXE) $(RUN_ARGS)
endif

.PHONY: clean
clean:
	$(RM) $(wildcard *.o) $(wildcard *.d) $(TARGET)$(EXE) makeflags

.PHONY: vars
vars:
	@echo TARGET: $(TARGET)
	@echo VERSION: $(VERSION)
	@echo DEFS: $(DEFS)
	@echo SRC: $(SRC)
	@echo OBJECTS: $(OBJECTS)
	@echo MAKEFILE_LIST: $(MAKEFILE_LIST)

$(TARGET)$(EXE): $(OBJECTS)
	$(LINK.cpp) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(OBJECTS): $(MAKEFILE_LIST) # rebuild if MAKEFILEs change

$(OBJECTS): makeflags # rebuild if MAKEFLAGS change
# Select only command line variables
cvars = _$(strip $(foreach flag,$(MAKEFLAGS),$(if $(findstring =,$(flag)),$(flag),)))_
makeflags: FORCE
	@[ "$(if $(wildcard $@),$(shell cat $@),)" = "$(cvars)" ] || echo $(cvars)> $@
FORCE: ;

# Establish module specific dependencies
-include $(OBJECTS:.o=.d)
