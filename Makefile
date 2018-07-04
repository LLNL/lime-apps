#
# $Id: $
#
# Description: Makefile for apps
#
# $Log: $
#

PACKAGE = apps-1.7.1
build=x86_64
HWP = $(WORKSPACE_LOC)/hw_platform_0
#XSDB = xmd$(if $(findstring Linux,$(shell uname -s)),,.bat) -tcl
XSDB = xsdb$(if $(findstring Linux,$(shell uname -s)),,.bat)

.PHONY: all
all:
ifeq ($(or $(findstring CLIENT,$(D)),$(findstring USE_LSU,$(D))),)
	cd bfs/$(build) && $(MAKE) $@
	cd dgemm/$(build) && $(MAKE) $@
	cd sort/$(build) && $(MAKE) $@
	cd strm/$(build) && $(MAKE) $@
	cd xsb/$(build) && $(MAKE) $@
endif
	cd image/$(build) && $(MAKE) $@
	cd pager/$(build) && $(MAKE) $@
	cd randa/$(build) && $(MAKE) $@
	cd rtb/$(build) && $(MAKE) $@
	cd spmv/$(build) && $(MAKE) $@

.PHONY: bfs
bfs:
	cd bfs/$(build) && $(MAKE) all

.PHONY: dgemm
dgemm:
	cd dgemm/$(build) && $(MAKE) all

.PHONY: image
image:
	cd image/$(build) && $(MAKE) all

.PHONY: pager
pager:
	cd pager/$(build) && $(MAKE) all

.PHONY: randa
randa:
	cd randa/$(build) && $(MAKE) all

.PHONY: rtb
rtb:
	cd rtb/$(build) && $(MAKE) all

.PHONY: sort
sort:
	cd sort/$(build) && $(MAKE) all

.PHONY: spmv
spmv:
	cd spmv/$(build) && $(MAKE) all

.PHONY: strm
strm:
	cd strm/$(build) && $(MAKE) all

.PHONY: xsb
xsb:
	cd xsb/$(build) && $(MAKE) all

.PHONY: dist
dist: distclean
	tar --transform 's,^,$(PACKAGE)/,' -czf ../$(PACKAGE).tgz --exclude-vcs .project *

.PHONY: distclean
distclean:
	$(RM) ../$(PACKAGE).tgz
	$(MAKE) build=arm_64 clean
	$(MAKE) build=x86_64 clean
	$(MAKE) build=zup clean
	$(MAKE) build=zynq clean

.PHONY: fpga
fpga:
ifeq ($(build),zynq)
	$(XSDB) misc/sdk/fpga_config_z7.tcl $(HWP)
else ifeq ($(build),zup)
	$(XSDB) misc/sdk/fpga_config_zu.tcl $(HWP)
endif

.DEFAULT:
ifeq ($(or $(findstring CLIENT,$(D)),$(findstring USE_LSU,$(D))),)
	cd bfs/$(build) && $(MAKE) $@
	cd dgemm/$(build) && $(MAKE) $@
	cd sort/$(build) && $(MAKE) $@
	cd strm/$(build) && $(MAKE) $@
	cd xsb/$(build) && $(MAKE) $@
endif
	cd image/$(build) && $(MAKE) $@
	cd pager/$(build) && $(MAKE) $@
	cd randa/$(build) && $(MAKE) $@
	cd rtb/$(build) && $(MAKE) $@
	cd spmv/$(build) && $(MAKE) $@
