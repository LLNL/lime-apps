LABEL = V$(subst .,_,$(VERSION))

# Cancel version control implicit rules
%:: %,v
%:: RCS/%
%:: RCS/%,v
%:: s.%
%:: SCCS/s.%
# Delete default suffixes
.SUFFIXES:
# Define suffixes of interest
.SUFFIXES: .o .c .cc .cpp .h .hpp .d .mak .ld

ifdef D
  SEP := ,
  DEFS += $(patsubst %,-D%,$(subst $(SEP), ,$(D)))
endif

# See shared/config.h
NEED_STREAM = $(filter %CLIENT %SERVER %OFFLOAD,$(DEFS))
NEED_ACC = $(filter %DIRECT %CLIENT %SERVER %OFFLOAD,$(DEFS))
