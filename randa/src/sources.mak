TARGET = randa
VERSION = 1.1
SRC = ../src ../../shared
ifneq (,$(filter %SERVER,$(DEFS)))
  DEFS += -DUSE_LSU
  MODULES = server
else
  MODULES = randa core_single_cpu_lcg utility
endif
ifneq (,$(filter %CLIENT %SERVER %USE_LSU,$(DEFS)))
  DEFS += -DUSE_SP -DUSE_OCM
  MODULES += lsu_cmd aport stream
endif
RUN_ARGS = 
# CXXFLAGS += -std=c++11
# LDFLAGS += 
# LDLIBS += -lstdc++
