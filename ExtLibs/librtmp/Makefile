include ../../config.local
ROOT_DIR := $(shell pwd)

#GPUL:=arm-hisiv300-linux-
#CC:= $(GPUL)gcc
CFLAGS += -Wall -O2 
CFLAGS += -lstdc++ -lpthread -lm -lrt
#AR = $(GPUL)ar
RM := rm -rf
SRC_DIR          +=   $(ROOT_DIR)/src
RTSPINCLUDE_DIR  += $(ROOT_DIR)/include
RTSPINCLUDE_DIR  += $(ROOT_DIR)/src
OUTINCLUDEDIR := ../../Include/rtmp
OUT_LIB_DIR := $(ROOT_DIR)/lib
SRCS                :=  $(shell find $(SRC_DIR) -name '*.c')
SRCS                :=  $(sort $(SRCS))

INCLUDES            :=  $(shell find $(RTSPINCLUDE_DIR) -name '*.h*')
INCLUDES         :=  $(sort $(dir $(INCLUDES)))
INCLUDES      :=  $(foreach dir,$(INCLUDES), -I$(dir))
#INCLUDE_PATH       :=  $(foreach dir,$(INCLUDE_DIR), -I$(dir))

OBJS                :=  $(SRCS:%.c=%.o)
COMPILE         =  $(CC) $(CFLAGS) -c "$<" -o "$@"  -I../../Include/ $(INCLUDES) 
LIB_OBJECT := libmtrtmp.a
.PHONY:all

all:$(LIB_OBJECT)

$(LIB_OBJECT):$(OBJS)

#	$(GPULS) $(OBJS) $(SHARED) -fPIC -o $(LIB_OBJECT)
	$(AR)	 rcs $(LIB_OBJECT)	$(OBJS)
	cp $(LIB_OBJECT) $(OUT_LIB_DIR) 
	cp  -rf $(ROOT_DIR)/include  $(OUT_LIB_DIR)
	cp  -rf $(ROOT_DIR)/include/* $(OUTINCLUDEDIR)
	cp $(LIB_OBJECT) ../../$(LIBDIR)/
#	cp $(LIB_OBJECT) $(TOP_DIR)/soc/mpp/lib

$(OBJS) :%.o : %.c
	@echo complie $@
	$(COMPILE)

clean:
	$(RM) $(OUT_LIB_DIR)/$(LIB_OBJECT) $(OBJS) 
	$(RM) ../../$(LIBDIR)/$(LIB_OBJECT)
	$(RM) $(OUT_LIB_DIR)/*
	$(RM) $(OUTINCLUDEDIR)/*