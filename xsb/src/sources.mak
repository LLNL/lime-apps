#===============================================================================
# User Options
#===============================================================================

DEBUG       = no
PROFILE     = no
MPI         = no
PAPI        = no
VEC_INFO    = no
VERIFY      = no
BENCHMARK   = no
BINARY_DUMP = no
BINARY_READ = no

#===============================================================================
# Program name & source code list
#===============================================================================

TARGET = xsb
VERSION = 16
SRC += ../src $(SHARED)
MODULES += \
Main \
io \
CalculateXS \
GridInit \
XSutils \
Materials

RUN_ARGS = -s small

#===============================================================================
# Sets Flags
#===============================================================================

# Standard Flags
CFLAGS += -std=gnu99

# Linker Flags
LDFLAGS += -lm

# Debug Flags
ifeq ($(DEBUG),yes)
  CFLAGS += -g
  LDFLAGS += -g
endif

# Profiling Flags
ifeq ($(PROFILE),yes)
  CFLAGS += -pg
  LDFLAGS += -pg
endif

# Compiler Vectorization (needs -O3 flag) information
ifeq ($(VEC_INFO),yes)
  CFLAGS += -ftree-vectorizer-verbose=6
endif

# PAPI source (you may need to provide -I and -L pointing
# to PAPI depending on your installation
ifeq ($(PAPI),yes)
  MODULES += papi
  DEFS += -DPAPI
  #CPPFLAGS += -I/soft/apps/packages/papi/papi-5.1.1/include
  #LDFLAGS += -L/soft/apps/packages/papi/papi-5.1.1/lib -lpapi
  LDFLAGS += -lpapi
endif

# Verification of results mode
ifeq ($(VERIFY),yes)
  DEFS += -DVERIFICATION
endif

# Adds outer 'benchmarking' loop to do multiple trials for
# 1 < threads <= max_threads
ifeq ($(BENCHMARK),yes)
  DEFS += -DBENCHMARK
endif

# Binary dump for file I/O based initialization
ifeq ($(BINARY_DUMP),yes)
  DEFS += -DBINARY_DUMP
endif

# Binary read for file I/O based initialization
ifeq ($(BINARY_READ),yes)
  DEFS += -DBINARY_READ
endif
