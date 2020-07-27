
PACKAGE = lime-apps-2.3.0

# Cancel version control implicit rules
%:: %,v
%:: RCS/%
%:: RCS/%,v
%:: s.%
%:: SCCS/s.%
# Delete default suffixes
.SUFFIXES:

build = x86_64
SEP := ,
DS = $(subst $(SEP), ,$(D))
APPS = bfs dgemm image kvgen ngram pager randa rtb sort spmv strm xsb

.PHONY: all
all .DEFAULT:
# No current support for accelerator
ifeq ($(filter CLIENT OFFLOAD,$(DS)),)
	$(MAKE) -C bfs/$(build) $@
	$(MAKE) -C dgemm/$(build) $@
	$(MAKE) -C sort/$(build) $@
	$(MAKE) -C strm/$(build) $@
	$(MAKE) -C xsb/$(build) $@
endif
# No support for MCU (MicroBlaze)
ifeq ($(filter CLIENT,$(DS)),)
	$(MAKE) -C rtb/$(build) $@
endif
# Not part of run suite. Can be run individually.
ifneq ($(filter clean,$(MAKECMDGOALS)),)
	$(MAKE) -C kvgen/$(build) $@
	$(MAKE) -C ngram/$(build) $@
endif
	$(MAKE) -C image/$(build) $@
	$(MAKE) -C pager/$(build) $@
	$(MAKE) -C randa/$(build) $@
	$(MAKE) -C spmv/$(build) $@

.PHONY: help
help:
	@echo "Use the make variable 'build' to specify the executable type"
	@echo "and 'D' to specify comma separated compile time defines."
	@echo "Use 'RUN_ARGS' to specify main arguments in Standalone OS."
	@echo "Applications can also be built individually by typing 'make'"
	@echo "within one of the build directories (e.g., rtb/zup)."
	@echo -e "\nTargets:"
	@echo "  <app>     - Build specific application:"
	@echo "              $(APPS)"
	@echo "              e.g., make build=arm_64 randa"
	@echo "  all       - Build application set"
	@echo "  clean     - Remove files for specified build type"
	@echo "  dist      - Package the release for distribution (tar)"
	@echo "  distclean - Remove files for all build types and distribution"
	@echo "  fpga      - Download bitfile to FPGA (Standalone only)"
	@echo "  run       - Build and run application set"
	@echo "              e.g., make build=arm_64 run"
	@echo -e "\nbuild=<type>"
	@echo "  arm_64 - ARM : Linux"
	@echo "  x86_64 - x86 : Linux (default)"
	@echo "  zup    - ARM : Zynq UltraScale+ : Standalone"
	@echo "  zynq   - ARM : Zynq 7000        : Standalone"
	@echo "  e.g., make build=arm_64"
	@echo -e "\nD=DEF1[=VAL1],DEF2[=VAL2],..."
	@echo "  -- LiME Configuration Options --"
	@echo "  CLOCKS - enable clock scaling for emulation"
	@echo "  STATS  - print memory access statistics"
	@echo "  TRACE  - enable trace capture, =_TADDR_, =_TALL_"
	@echo "           _TADDR_ - capture only R/W address AXI events (default)"
	@echo "           _TALL_  - capture all AXI events"
	@echo "  USE_SD - use SD card for trace capture in Standalone OS"
	@echo "  -- Accelerator Options --"
	@echo "  CLIENT  - enable protocol for sending commands to server (MCU)"
	@echo "  DIRECT  - host executes accelerator algorithm with direct calls"
	@echo "  OFFLOAD - offload work from host to accelerator (no MCU)"
	@echo "  STOCK   - host executes stock algorithm with no accelerator"
	@echo "  e.g., make D=CLOCKS,STATS,TRACE=_TALL_,CLIENT"
	@echo -e "\nRUN_ARGS=\"-arg1 -arg2 ...\""
	@echo "  e.g., make RUN_ARGS=\"-e32Mi -l.60 -c -w1Mi -h.90 -z.99\""
	@echo -e "\nARG=<str>"
	@echo "  <null> - use default argument set"
	@echo "  1      - use alternative argument set (larger data)"
	@echo "  e.g., make ARG=1"
	@echo -e "\nPreconditions:"
	@echo "  1) Xilinx tools in path"
	@echo "     e.g., source /opt/Xilinx/Vivado/<version>/settings64.sh"
	@echo "  -- Standalone Only --"
	@echo "  2) Specify location of lime directory"
	@echo "     e.g., export LIME=\$$HOME/<mywork>/lime"
	@echo "  3) Specify location of Boost C++ libraries"
	@echo "     e.g., export BOOST_ROOT=\$$HOME/<mysrc>/boost_1_xx_0"

.PHONY: $(APPS)
$(APPS):
	$(MAKE) -C $@/$(build) all

.PHONY: dist
dist: distclean
	tar --transform 's,^,$(PACKAGE)/,' -czf ../$(PACKAGE).tgz --exclude-vcs *

.PHONY: distclean
distclean:
	$(RM) ../$(PACKAGE).tgz
	$(MAKE) build=arm_64 clean
	$(MAKE) build=x86_64 clean
	$(MAKE) build=zup clean
	$(MAKE) build=zynq clean

search = $(firstword $(wildcard $(addsuffix /$(2),$(subst ;, ,$(1)))))
MAKDIR := $(call search,$(LIME);.;../lime,make)
ifeq ($(MAKDIR),)
  $(error LIME root directory not found or defined)
endif
LIME := $(patsubst %/,%,$(dir $(MAKDIR)))
WORKSPACE_LOC ?= $(LIME)/standalone/sdk
HWP = $(WORKSPACE_LOC)/hw_platform_0
XSDB = xsdb$(if $(findstring Linux,$(shell uname -s)),,.bat)

.PHONY: fpga
fpga:
ifeq ($(build),zynq)
	$(XSDB) $(MAKDIR)/sdk/fpga_config_z7.tcl $(HWP)
else ifeq ($(build),zup)
	$(XSDB) $(MAKDIR)/sdk/fpga_config_zu.tcl $(HWP)
else
	@echo "specify build=zup|zynq"
endif
