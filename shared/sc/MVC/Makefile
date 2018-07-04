#
# $Id: $
#
# Description: Makefile for stest program
#
# $Log: $
#

# comma separated list of defines
ifdef D
  SEP := ,
  DEFS += $(patsubst %,-D%,$(subst $(SEP), ,$(D)))
endif

########## beg: sources.mak ##########
TARGET = stest
VERSION = 1.0
SRC = . ..
MODULES = kvs
MODULES += stest stream_sc aport_sc
########## end: sources.mak ##########

include ../../misc/make/x86_64.mak
