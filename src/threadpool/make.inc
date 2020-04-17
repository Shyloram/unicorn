ZMD_INCLUDE += -I$(ZMD_INC_ROOT)/threadpool/

ifeq ($(ZMD_APP_THREADPOOL),y)
ZMD_SRCS += $(wildcard $(ZMD_SRC_ROOT)/threadpool/*.c)
endif
