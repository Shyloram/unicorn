##
## Zmodo App Main Makefile
##

.PHONY: all
all:

ZMD_APP_TOPDIR   := $(PWD)
ZMD_SCRIPT_ROOT  := $(ZMD_APP_TOPDIR)/script
ZMD_TOOLS_ROOT   := $(ZMD_APP_TOPDIR)/tools
ZMD_SRC_ROOT     := $(ZMD_APP_TOPDIR)/src
ZMD_INC_ROOT     := $(ZMD_APP_TOPDIR)/inc
ZMD_LIB_ROOT     := $(ZMD_APP_TOPDIR)/lib
ZMD_TIR_ROOT     := $(ZMD_APP_TOPDIR)/thirdparty

ZMD_SRCS         :=
ZMD_CPPSRCS      :=
ZMD_INCLUDE      := -I$(ZMD_APP_TOPDIR)/
ZMD_LIB          :=
ZMD_TIR_LIB      :=

include $(ZMD_APP_TOPDIR)/build/Kconfig.mk

DOT_CONFIG     := $(ZMD_APP_TOPDIR)/.config
ifeq ($(wildcard $(DOT_CONFIG)),$(DOT_CONFIG))
include $(DOT_CONFIG)
else
$(warning Can not found .config. You need to make menuconfig and save it into .config first!)
endif

#Find all of Makefile
MAKE_INC       := $(shell find $(ZMD_APP_TOPDIR)/ -name 'make.inc')
include $(MAKE_INC)

ifeq ($(ZMD_APP_HISI),y)
ifeq ($(ZMD_APP_HISI_LITEOS),y)
include $(ZMD_APP_TOPDIR)/mk/liteos.mk
endif
ifeq ($(ZMD_APP_HISI_LINUX),y)
include $(ZMD_APP_TOPDIR)/mk/linux.mk
endif
endif
