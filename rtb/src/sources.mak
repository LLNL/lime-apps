TARGET = rtb
VERSION = 3.1
SRC = ../src ../../shared
ifneq (,$(filter %SERVER,$(DEFS)))
  MODULES = server
else
  MODULES = rtb fasta path
  ifneq (,$(filter zup zynq,$(notdir $(CURDIR))))
    MODULES += fatfs
  endif
endif
ifneq (,$(filter %USE_LSU,$(DEFS)))
  DEFS += -DUSE_HASH
  MODULES += lsu_cmd
endif
ifneq (,$(filter %DIRECT %CLIENT %SERVER %USE_HASH,$(DEFS)))
  MODULES += short
endif
ifneq (,$(filter %CLIENT %SERVER %USE_HASH,$(DEFS)))
  DEFS += -DUSE_SP -DUSE_OCM
  MODULES += aport stream
endif
RUN_ARGS = -e32Mi -l.60 -c -w1Mi -h.90 -z.99
# CXXFLAGS += -std=c++11
# LDFLAGS += 
# LDLIBS += -lstdc++
