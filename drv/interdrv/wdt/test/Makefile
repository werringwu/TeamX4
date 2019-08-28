ifeq ($(PARAM_FILE), )
        PARAM_FILE:=../../../../mpp/Makefile.param
        include $(PARAM_FILE)
endif

CFLAGS += -DCHIP_TYPE_$(HIARCH)

SRC_ROOT :=$(PWD)

INC_FLAGS += -I$(SRC_ROOT)/../ -I$(REL_DIR)/include
CFLAGS += -Wall -g $(INC_FLAGS) -lpthread

ifeq ($(OSTYPE),liteos)
CFLAGS += -I$(SRC_ROOT)/../init/liteos
else
LIBA := -L$(SRC_ROOT)/../  -L$(REL_DIR)/lib
endif

TARGET := wtdg_test
SRCS  := $(wildcard *.c)
HEAD := $(wildcard *.h)

# compile linux or LiteOS
include $(OSTYPE).mak
