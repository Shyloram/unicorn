ifeq ($(ZMD_APP_THI_JSON),y)
ZMD_INCLUDE += -I$(ZMD_TIR_ROOT)/json-c-0.13/include/json-c
ZMD_TIR_LIB += $(wildcard $(ZMD_TIR_ROOT)/json-c-0.13/lib/*.a)
endif
