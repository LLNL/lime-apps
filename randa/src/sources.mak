TARGET = randa
VERSION = 1.1
SRC += ../src $(SHARED)
ifneq ($(filter %SERVER,$(DEFS)),)
  MODULES += server
else
  MODULES += randa core_single_cpu_lcg utility
endif
ifneq ($(filter $(NEED_STREAM),$(DEFS)),)
  DEFS += -DUSE_LSU
  MODULES += lsu_cmd
endif
ifeq ($(ARG),1)
  DEFS += -DVECT_LEN=65536
  RUN_ARGS = -s31
else
  RUN_ARGS = -s29
endif
# CXXFLAGS += -std=c++11
# LDFLAGS += 
# LDLIBS += -lstdc++
