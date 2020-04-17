ifeq ($(ZMD_APP_THI_CURL),y)
ZMD_INCLUDE += -I$(ZMD_TIR_ROOT)/curl-7.59.0/include/curl
ZMD_TIR_LIB += $(wildcard $(ZMD_TIR_ROOT)/curl-7.59.0/lib/*.a)
endif
