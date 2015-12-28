#
# $Id: $
#
# Description: Makefile for apps
#
# $Log: $
#

PACKAGE = apps-1.4
build=x86_64

.PHONY: all
all:
	cd bfs/$(build) && $(MAKE) $@
	cd dgemm/$(build) && $(MAKE) $@
	cd image/$(build) && $(MAKE) $@
	cd pager/$(build) && $(MAKE) $@
	cd randa/$(build) && $(MAKE) $@
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

.DEFAULT:
	cd bfs/$(build) && $(MAKE) $@
	cd dgemm/$(build) && $(MAKE) $@
	cd image/$(build) && $(MAKE) $@
	cd pager/$(build) && $(MAKE) $@
	cd randa/$(build) && $(MAKE) $@
	cd spmv/$(build) && $(MAKE) $@
	cd strm/$(build) && $(MAKE) $@
