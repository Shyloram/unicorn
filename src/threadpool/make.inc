ZMD_INCLUDE += -I$(ZMD_INC_ROOT)/threadpool/
ZMD_CPPSRCS += $(ZMD_SRC_ROOT)/threadpool/thpoolinterface.cpp

ifeq ($(ZMD_APP_THREADPOOL),y)

ZMD_CPPSRCS += $(ZMD_SRC_ROOT)/threadpool/threadpool.cpp

endif
