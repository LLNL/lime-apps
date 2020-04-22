TARGET = sort
VERSION = 1.0
SRC += ../src $(SHARED)
MODULES += sort
ifeq ($(ARG),1)
  RUN_ARGS = -c -s28
else
  RUN_ARGS = -c -s20
endif
# CXXFLAGS += -std=c++11
# LDFLAGS += 
LDLIBS += -lstdc++
