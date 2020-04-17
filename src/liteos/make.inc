ifeq ($(ZMD_APP_HISI_LITEOS),y)
ZMD_INCLUDE += -I$(ZMD_INC_ROOT)/liteos/common/
ZMD_INCLUDE += -I$(ZMD_INC_ROOT)/liteos/mpp/
ZMD_INCLUDE += -I$(ZMD_INC_ROOT)/liteos/osal/
ZMD_INCLUDE += -I$(ZMD_INC_ROOT)/liteos/sensor_spi/
ZMD_INCLUDE += -I$(ZMD_INC_ROOT)/liteos/isp/control/include
ZMD_INCLUDE += -I$(ZMD_INC_ROOT)/liteos/isp/stream/include
ZMD_SRCS += $(wildcard $(ZMD_SRC_ROOT)/liteos/*.c)
endif
