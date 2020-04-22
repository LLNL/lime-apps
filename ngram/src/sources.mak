TARGET = ngram
VERSION = 1.0
SRC += ../src $(SHARED)
ifneq ($(filter %SERVER,$(DEFS)),)
  MODULES += server
else
  MODULES += ngram path
  ifneq ($(filter zup zynq,$(notdir $(CURDIR))),)
    MODULES += fatfs
  endif
endif
ifneq ($(filter $(NEED_ACC),$(DEFS)),)
  MODULES += short
endif
ifneq ($(filter $(NEED_STREAM),$(DEFS)),)
  DEFS += -DUSE_HASH
  ifneq ($(filter %SYSTEMC,$(DEFS)),)
    MODULES += kvs
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
