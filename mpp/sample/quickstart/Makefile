include ../Makefile.param

LOCAL_DIR = $(shell pwd)
SCENE_DIR = $(LOCAL_DIR)/../scene_auto
INC_FLAGS :=-I$(SCENE_DIR)/include
INC_FLAGS +=-I$(SCENE_DIR)/tools/configaccess/include
INC_FLAGS +=-I$(SCENE_DIR)/tools/iniparser/include
INC_FLAGS +=-I$(SCENE_DIR)/tools/hilog/include
INC_FLAGS +=-I./../common
INC_FLAGS +=-I$(REL_DIR)/include

SRCS += $(wildcard $(LOCAL_DIR)/src/*.c)

SRCS += $(wildcard $(SCENE_DIR)/tools/configaccess/src/*.c)
SRCS += $(wildcard $(SCENE_DIR)/tools/iniparser/src/*.c)
SRCS += $(wildcard $(SCENE_DIR)/tools/hilog/src/*.c)
SRCS += $(wildcard $(SCENE_DIR)/src/core/*.c)
SRCS += $(wildcard $(SCENE_DIR)/src/sample/scene_loadparam.c)

.PHONY : clean all
# target source
TARGET_PATH := $(PWD)
TARGET := sample_quickstart
#OBJS  := $(SRCS:%.c=%.o)
CFLAGS += $(INC_FLAGS)

# all: $(TARGET)

# $(TARGET):$(OBJS)
# 	@$(CC) $(CFLAGS) -lpthread -lm -o $@ $^ -Wl,--start-group $(MPI_LIBS) $(SENSOR_LIBS) $(AUDIO_LIBA) $(REL_LIB)/libsecurec.a -Wl,--end-group

# clean:
# 	@rm -f $(TARGET)
# 	@rm -f $(OBJS)


# include ../Makefile.param

# SRCS := $(wildcard *.c)

# TARGET := $(SRCS:%.c=%)

# CFLAGS += -L$(REL_LIB) -lsecurec -Wl,-rpath=/usr/lib

# SRCS += $(wildcard $(PWD)/adp/*.c)

# TARGET_PATH := $(PWD)

# compile linux or HuaweiLite
include $(PWD)/../$(ARM_ARCH)_$(OSTYPE).mak
