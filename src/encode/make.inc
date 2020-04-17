ZMD_INCLUDE += -I$(ZMD_INC_ROOT)/encode/
ZMD_CPPSRCS += $(ZMD_SRC_ROOT)/encode/encodeinterface.cpp 

ifeq ($(ZMD_APP_ENCODE),y)

ZMD_CPPSRCS += $(ZMD_SRC_ROOT)/encode/video.cpp 
ZMD_CPPSRCS += $(ZMD_SRC_ROOT)/encode/buffermanage.cpp

ifeq ($(ZMD_APP_ENCODE_AUDIO),y)
ZMD_CPPSRCS += $(ZMD_SRC_ROOT)/encode/audio.cpp
endif

endif
