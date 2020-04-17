ZMD_INCLUDE += -I$(ZMD_INC_ROOT)/rwConfig/

ZMD_CPPSRCS += $(ZMD_SRC_ROOT)/rwConfig/rwconfiginterface.cpp

ifeq ($(ZMD_APP_RWCONDIF),y)
ZMD_CPPSRCS += $(ZMD_SRC_ROOT)/rwConfig/rwconfig.cpp
endif
