#
# isp firmware Makefile
#

ifeq ($(PARAM_FILE), )
    PARAM_FILE:=../../../../Makefile.param
    include $(PARAM_FILE)
endif

ifeq ($(CBB_PARAM_FILE), )
    CBB_PARAM_FILE:=../../../Makefile.param
    include $(CBB_PARAM_FILE)
endif

ISP_PATH := $(ISP_ROOT)
LIBPATH = $(ISP_PATH)/user/lib
OBJPATH = ./obj

3A_INC  := $(ISP_PATH)/user/3a/include
VREG_INC := $(ISP_PATH)/firmware/vreg
DRC_INC := $(ISP_PATH)/drc/

#tmp, delete in the future
SRC_INC := $(ISP_PATH)/user/firmware/src/main
SRC_INC += -I$(ISP_PATH)/user/firmware/src/algorithms
SRC_INC += -I$(ISP_PATH)/user/firmware/src/arch/$(HIARCH)/hal

ISP_INC := -I$(ISP_PATH)/user/include
ISP_INC += -I$(ISP_PATH)/user/firmware/include
ISP_INC += -I$(ISP_PATH)/user/firmware/arch/include
ISP_INC += -I$(ISP_PATH)/user/firmware/arch/$(HIARCH)/include

ISP_INCLUDE := $(ISP_PATH)/include
ISP_MKP := $(ISP_PATH)/kernel/mkp/include
ISP_MKP += -I$(ISP_PATH)/kernel/arch/include
ISP_MKP += -I$(ISP_PATH)/kernel/arch/$(HIARCH)/include

ifeq ($(MPP_BUILD), y)
EXT_PATH := $(SDK_PATH)/mpp/$(EXTDRV)
else
EXT_PATH := $(SDK_PATH)/mpp/extdrv
endif

BUS_DIR := $(EXT_PATH)/pwm

ARFLAGS = rcv
CFLAGS += -Wall -fPIC

ifeq ($(HIGDB),HI_GDB)
CFLAGS += -g
endif

ifeq ($(CONFIG_JPEGEDCF), y)
     CFLAGS += -D ENABLE_JPEGEDCF
endif

CFLAGS  += -O2
CFLAGS  += $(LIBS_CFLAGS)
DFLAGS  := -DEXT_REG

INC := $(COMM_INC) -I$(ISP_INCLUDE) -I$(ISP_INC) -I$(ISP_MKP) -I$(BUS_DIR) -I$(3A_INC) -I$(SRC_INC) -I$(VREG_INC) -I$(DRC_INC)

DEPEND_FILE := $(foreach file,$(subst -I, ,$(INC)),$(wildcard $(file)/*.h))
COMPILE = $(CC) $(CFLAGS) $(DFLAGS) -I$(ISP_MKP) -I$(ISP_INC) -lm

$(OBJPATH)/%.o: ./%.c $(DEPEND_FILE)
	@(echo "compiling $< ...")
	@[ -e $(LIBPATH) ] || mkdir -p $(LIBPATH)
	@[ -e $(OBJPATH) ] || mkdir -p $(OBJPATH)
	@($(COMPILE) -o $@ -c $< $(INC))

SRCS = $(wildcard ./*.c)
OBJS = $(SRCS:%.c=%.o)
OBJS := $(OBJS:./%=obj/%)

LIB_A := $(LIBPATH)/lib_hiir_auto.a
LIB_S := $(LIBPATH)/lib_hiir_auto.so

all:$(OBJS)
	@($(AR) $(ARFLAGS) $(LIB_A) $(OBJS))
	@($(CC) $(LIBS_LD_CFLAGS) -shared -fPIC -o $(LIB_S) $(OBJS))
	@mkdir -p $(REL_LIB)
	@cp $(LIB_S) $(REL_LIB)
	@cp $(LIB_A) $(REL_LIB)
clean:
	@$(RM) -rf $(LIB_A) $(LIB_S) $(OBJS)
	@$(RM) -rf $(OBJPATH)
