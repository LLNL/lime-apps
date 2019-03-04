TARGET = rtb
VERSION = 3.1
SRC = ../src ../../shared
ifneq ($(filter %SYSTEMC,$(DEFS)),)
  SRC += ../../shared/sc
  LDFLAGS += -static
endif
ifneq ($(filter %SERVER,$(DEFS)),)
  MODULES = server
else
  MODULES = rtb path
  ifneq ($(filter zup zynq,$(notdir $(CURDIR))),)
    MODULES += fatfs
  endif
endif
ifneq ($(filter %USE_LSU,$(DEFS)),)
  DEFS += -DUSE_HASH
  MODULES += lsu_cmd
endif
ifneq ($(filter %DIRECT %CLIENT %SERVER %USE_HASH,$(DEFS)),)
  MODULES += short
endif
ifneq ($(filter %CLIENT %SERVER %USE_HASH,$(DEFS)),)
  ifneq ($(filter %SYSTEMC,$(DEFS)),)
    MODULES += aport stream_sc kvs
  else
    DEFS += -DUSE_SP -DUSE_OCM
    MODULES += aport stream
  endif
endif
ifneq ($(filter %SYSTEMC,$(DEFS)),)
  RUN_ARGS = -e32Mi -l.60 -c -w8Ki -h.90 -z.99
else ifeq ($(ARG),1)
  RUN_ARGS = -e32Mi -l.60 -c -w1Mi -h.90 -z.99 -b20
else
  RUN_ARGS = -e32Mi -l.60 -c -w1Mi -h.90 -z.99
endif
CXXFLAGS += -std=c++11
# LDFLAGS += 
# LDLIBS += -lstdc++
