TARGET = pager
VERSION = 1.1
SRC += ../src $(SHARED)
ifneq ($(filter %SERVER,$(DEFS)),)
  MODULES += server
else
  MODULES += pager
  ifdef BOOST_ROOT
    CPPFLAGS += -I$(BOOST_ROOT)
  else ifneq ($(wildcard $(HOME)/local/include/boost),)
    CPPFLAGS += -I$(HOME)/local/include
  endif
  LDLIBS += -lstdc++
endif
ifneq ($(filter $(NEED_STREAM),$(DEFS)),)
  DEFS += -DUSE_LSU
  MODULES += lsu_cmd
  ifeq ($(filter zup zynq,$(notdir $(CURDIR))),)
    MODULES += dlmalloc
  endif
endif
ifeq ($(ARG),1)
  RUN_ARGS = -s21 -v15
else
  RUN_ARGS = -s18 -v15
endif
# CXXFLAGS += -std=c++11
# LDFLAGS += 
