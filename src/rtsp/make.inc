ZMD_INCLUDE += -I$(ZMD_INC_ROOT)/rtsp/
ZMD_CPPSRCS += $(ZMD_SRC_ROOT)/rtsp/rtspinterface.cpp 

ifeq ($(ZMD_APP_RTSP),y)

ZMD_CPPSRCS += $(ZMD_SRC_ROOT)/rtsp/rtp.cpp 
ZMD_CPPSRCS += $(ZMD_SRC_ROOT)/rtsp/rtspserver.cpp

endif
