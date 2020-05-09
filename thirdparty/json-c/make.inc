ifeq ($(ZMD_APP_THI_JSON),y)
ZMD_INCLUDE += -I$(ZMD_TIR_ROOT)/json-c/include/json
ZMD_TIR_LIB += $(wildcard $(ZMD_TIR_ROOT)/json-c/lib/*.a)
endif
