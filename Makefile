#
# $Id: $
#
# Description: Makefile for apps
#
# $Log: $
#

PACKAGE = apps-1.5
build=x86_64

.PHONY: all
all:
	cd bfs/$(build) && $(MAKE) $@
	cd dgemm/$(build) && $(MAKE) $@
	cd image/$(build) && $(MAKE) $@
	cd pager/$(build) && $(MAKE) $@
	cd randa/$(build) && $(MAKE) $@
	cd rtb/$(build) && $(MAKE) $@
	cd sort/$(build) && $(MAKE) $@
	cd spmv/$(build) && $(MAKE) $@
	cd strm/$(build) && $(MAKE) $@

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

.PHONY: dist
dist: distclean
	tar --transform 's,^,$(PACKAGE)/,' -czf ../$(PACKAGE).tgz --exclude-vcs --exclude='*.o' --exclude='*.log' .project *

.PHONY: distclean
distclean:
	$(RM) ../$(PACKAGE).tgz
	$(MAKE) build=arm_64 clean
	$(MAKE) build=x86_64 clean
	$(MAKE) build=zup clean
	$(MAKE) build=zynq clean

.DEFAULT:
	cd bfs/$(build) && $(MAKE) $@
	cd dgemm/$(build) && $(MAKE) $@
	cd image/$(build) && $(MAKE) $@
	cd pager/$(build) && $(MAKE) $@
	cd randa/$(build) && $(MAKE) $@
	cd rtb/$(build) && $(MAKE) $@
	cd sort/$(build) && $(MAKE) $@
	cd spmv/$(build) && $(MAKE) $@
	cd strm/$(build) && $(MAKE) $@
