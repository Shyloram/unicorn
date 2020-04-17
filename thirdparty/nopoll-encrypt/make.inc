ifeq ($(ZMD_APP_THI_NOPOLL),y)
ZMD_INCLUDE += -I$(ZMD_TIR_ROOT)/nopoll-encrypt/include/
ZMD_TIR_LIB += $(wildcard $(ZMD_TIR_ROOT)/nopoll-encrypt/lib/*.a)
endif
