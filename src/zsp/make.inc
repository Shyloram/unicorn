ZMD_INCLUDE += -I$(ZMD_INC_ROOT)/zsp/
ZMD_INCLUDE += -I$(ZMD_INC_ROOT)/common/

ZMD_CPPSRCS += $(ZMD_SRC_ROOT)/zsp/zspinterface.cpp

ifeq ($(ZMD_APP_ZSP),y)
ZMD_CPPSRCS += $(ZMD_SRC_ROOT)/zsp/zsp_main.cpp
ZMD_CPPSRCS += $(ZMD_SRC_ROOT)/zsp/zsp_cmd_processor.cpp
endif
