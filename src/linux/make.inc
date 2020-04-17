ifeq ($(ZMD_APP_HISI_LINUX),y)
ZMD_INCLUDE += -I$(ZMD_INC_ROOT)/linux/mpp/
ZMD_INCLUDE += -I$(ZMD_INC_ROOT)/linux/common/
ZMD_SRCS    += $(wildcard $(ZMD_SRC_ROOT)/linux/*.c)
ZMD_CPPSRCS += $(ZMD_SRC_ROOT)/linux/main.cpp
endif
