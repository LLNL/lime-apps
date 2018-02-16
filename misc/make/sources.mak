TARGET = main
VERSION = 1.0
SRC = ../../shared ../src
ifneq (,$(filter %SERVER,$(DEFS)))
  DEFS += -DUSE_LSU
  MODULES = server
else
  MODULES = main mod1 mod2
endif
ifneq (,$(filter %DIRECT %CLIENT %SERVER %USE_LSU,$(DEFS)))
  MODULES += accelerator
endif
ifneq (,$(filter %CLIENT %SERVER %USE_LSU,$(DEFS)))
  DEFS += -DUSE_SP -DUSE_OCM
  MODULES += lsu_cmd aport stream
endif
CHECK_ARGS = -arg1 -arg2
# CXXFLAGS += -std=c++11
# LDFLAGS += 
# LDLIBS += -lstdc++
