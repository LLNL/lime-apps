TARGET = main
VERSION = 1.0
SRC += ../src ../../shared
ifneq ($(filter %SERVER,$(DEFS)),)
  MODULES += server
else
  MODULES += main mod1 mod2
endif
ifneq ($(NEED_ACC),)
  MODULES += accelerator
endif
ifneq ($(NEED_STREAM),)
  DEFS += -DUSE_LSU
  MODULES += lsu_cmd
endif
ifeq ($(ARG),1)
  RUN_ARGS = -arg1x -arg2x
else
  RUN_ARGS = -arg1 -arg2
endif
# CXXFLAGS += -std=c++11
# LDFLAGS += 
# LDLIBS += -lstdc++
