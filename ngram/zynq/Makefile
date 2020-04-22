search = $(firstword $(wildcard $(addsuffix /$(2),$(subst :, ,$(1)))))
MAKDIR := $(call search,$(LIME):../..:../../..:../../../lime,make)
ifeq ($(MAKDIR),)
  $(error LIME root directory not found or defined)
endif
include $(MAKDIR)/common.mak
include ../src/sources.mak
include $(MAKDIR)/$(notdir $(CURDIR)).mak
