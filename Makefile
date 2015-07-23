#
# $Id: $
#
# Description: Makefile for apps
#
# $Log: $
#

PACKAGE = apps-1.2
build=x86_64

.PHONY: all
all:
	cd image/$(build) && $(MAKE) $@
	cd pager/$(build) && $(MAKE) $@
	cd randa/$(build) && $(MAKE) $@
	cd spmv/$(build) && $(MAKE) $@
	cd strm/$(build) && $(MAKE) $@

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
	tar --transform 's,^,$(PACKAGE)/,' -czf ../$(PACKAGE).tgz .c* .p* .s* *

.PHONY: distclean
distclean:
	$(RM) -rf ../$(PACKAGE).tgz

.DEFAULT:
	cd image/$(build) && $(MAKE) $@
	cd pager/$(build) && $(MAKE) $@
	cd randa/$(build) && $(MAKE) $@
	cd spmv/$(build) && $(MAKE) $@
	cd strm/$(build) && $(MAKE) $@
