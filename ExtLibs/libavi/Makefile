ROOTDIR :=$(shell pwd)
include ../../config.local

LIB   :=Libavi.a
CPULS = arm-hisiv300-linux-gcc

SRC_DIR = $(ROOTDIR)/src
SRCS   :=  $(shell find $(SRC_DIR) -name '*.c')
SRCS   :=  $(sort $(SRCS))

INCLUDE_DIR         :=  $(ROOT_DIR)/include

INCLUDES            :=  $(shell find $(INCLUDESDIR) -name '*.h*')
INCLUDESDIR         :=  $(sort $(dir $(INCLUDES)))
INCLUDE_PATH       :=  $(foreach dir,$(INCLUDESDIR), -I$(dir))

OBJS                :=  $(SRCS:%.c=%.o)
all:$(LIB)
	
$(LIB) :$(OBJS)
	$(AR) -rc $(LIB) $(OBJS)
	$(CP) $(LIB) ../../$(LIBDIR)/
$(OBJS) :%.o : %.c
	$(CPULS)  $(BASE_CFLAGS)  \
	-g -c  $(INCLUDE_PATH) "$<" -o "$@" 

clean: 
	-rm -rf $(OBJS)
	-rm -rf $(LIB)
	-rm -rf ../../$(LIBDIR)/$(LIB)	