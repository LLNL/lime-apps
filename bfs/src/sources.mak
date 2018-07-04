TARGET = bfs
VERSION = 1.1
SRC = ../src ../../shared
ifneq ($(filter %SERVER,$(DEFS)),)
  DEFS += -DUSE_LSU
  MODULES = server
else
  MODULES = bfs
  ifdef BOOST_ROOT
    CPPFLAGS += -I$(BOOST_ROOT)
  else ifneq ($(wildcard $(HOME)/local/include/boost),)
    CPPFLAGS += -I$(HOME)/local/include
  endif
  LDLIBS += -lstdc++
endif
ifneq ($(filter %CLIENT %SERVER %USE_LSU,$(DEFS)),)
  DEFS += -DUSE_SP -DUSE_OCM
  MODULES += lsu_cmd aport stream
endif
ifeq ($(ARG),1)
  RUN_ARGS = -s21 -v15
else
  RUN_ARGS = -s18 -v15
endif
# CXXFLAGS += -std=c++11
# LDFLAGS += 
