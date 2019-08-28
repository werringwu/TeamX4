# Hisilicon Hi35xx sample Makefile

include ../Makefile.param

CFLAGS += -I$(PWD)/common
CFLAGS += -I$(PWD)/sample

SRCS := $(wildcard *.c)
SRCS += $(wildcard ./sample/*.c)
SRCS += $(wildcard ./common/*.c)

MPI_LIBS += $(REL_LIB)/libive.a
MPI_LIBS += $(REL_LIB)/libmd.a

TARGET := sample_ive_main

TARGET_PATH := $(PWD)

# compile linux or HuaweiLite
include $(PWD)/../$(ARM_ARCH)_$(OSTYPE).mak
