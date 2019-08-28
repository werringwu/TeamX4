# Hisilicon Hi35xx sample Makefile

include ../Makefile.param

SRCS := $(wildcard *.c)

TARGET := $(SRCS:%.c=%)

TARGET_PATH := $(PWD)

MPI_LIBS += $(REL_LIB)/libbcd.a
MPI_LIBS += $(REL_LIB)/libsecurec.a

# compile linux or HuaweiLite
include $(PWD)/../$(ARM_ARCH)_$(OSTYPE).mak
