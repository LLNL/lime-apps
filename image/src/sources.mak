TARGET = image
VERSION = 1.1
SRC += ../src $(SHARED)
DEFS += -DENTIRE
ifneq ($(filter %SERVER,$(DEFS)),)
  MODULES += server
else
  MODULES += image ColorImage
endif
ifneq ($(filter $(NEED_STREAM),$(DEFS)),)
  DEFS += -DUSE_LSU
  MODULES += lsu_cmd
endif
ifeq ($(ARG),1)
  RUN_ARGS = -d2 -v15 -w24000 -h16000 pat pat
else
  RUN_ARGS = -d16 -v15 -w16000 -h8000 pat pat
endif
# CXXFLAGS += -std=c++11
# LDFLAGS += 
# LDLIBS += -lstdc++
