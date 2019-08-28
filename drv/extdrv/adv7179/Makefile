ifeq ($(EXTDRV_PARAM_FILE), )
    EXTDRV_PARAM_FILE:=../Makefile.param
    include $(EXTDRV_PARAM_FILE)
endif

SRCS := adv7179.c
INC += -I$(OSAL_ROOT)/include
INC += -I$(DRV_ROOT)/extdrv/$(EXTDRVVER)/adv7179/arch/$(ARCH_DIR)/
ifeq ($(MPP_BUILD),y)
        INC += -I$(OSAL_ROOT)/$(OSTYPE)/kernel/himedia
else
        INC += -I$(OSAL_ROOT)/source/kernel/himedia
endif
EXTDRV_CFLAGS += $(INC)
EXTRA_CFLAGS += -DHI_I2C
EXTRA_CFLAGS += -DFPGA_UMAP7

#*************************************************************************
TARGET := hi_adv7179
#*************************************************************************
# compile linux or HuaweiLite
include $(PWD)/../Make.$(OSTYPE)
