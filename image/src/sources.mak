TARGET = image
VERSION = 1.1
SRC = ../src ../../shared
DEFS += -DENTIRE
ifneq ($(filter %SERVER,$(DEFS)),)
  DEFS += -DUSE_LSU
  MODULES = server
else
  MODULES = image ColorImage
endif
ifneq ($(filter %CLIENT %SERVER %USE_LSU,$(DEFS)),)
  DEFS += -DUSE_SP -DUSE_OCM
  MODULES += lsu_cmd aport stream
endif
ifeq ($(ARG),1)
  RUN_ARGS = -d2 -v15 -w24000 -h16000 pat pat
else
  RUN_ARGS = -d16 -v15 -w16000 -h8000 pat pat
endif
# CXXFLAGS += -std=c++11
# LDFLAGS += 
# LDLIBS += -lstdc++
