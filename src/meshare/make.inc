ZMD_INCLUDE += -I$(ZMD_INC_ROOT)/meshare/
ZMD_INCLUDE += -I$(ZMD_INC_ROOT)/common/

ZMD_CPPSRCS += $(ZMD_SRC_ROOT)/meshare/meshareinterface.cpp

ifeq ($(ZMD_APP_MESHARE),y)
ZMD_CPPSRCS += $(ZMD_SRC_ROOT)/meshare/meshare_main.cpp
ZMD_CPPSRCS += $(ZMD_SRC_ROOT)/meshare/http_client.cpp
ZMD_CPPSRCS += $(ZMD_SRC_ROOT)/meshare/websocket_client.cpp 
ZMD_CPPSRCS += $(ZMD_SRC_ROOT)/meshare/trans_client.cpp
endif
