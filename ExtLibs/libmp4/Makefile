export ROOT_DIR:=$(shell pwd)
include $(shell pwd)/../../config.local
TOP_DIR := ..

LIB_DIR := $(ROOT_DIR)/../libs
#OBJ_DIR := $(ROOT_DIR)/obj
SRC_DIR = $(ROOT_DIR)/src 
			

#$(shell mkdir -p ${LIB_DIR})

#$(shell mkdir -p ${OBJ_DIR})

RM = rm -rf

#****************************************************************************

CROSS_COMPILE =arm-hisiv300-linux-
GPULS = $(CROSS_COMPILE)g++
AR =  $(CROSS_COMPILE)ar
SHARED = -shared 

#FPIC = -fPIC -c


CXX_CFLAGS+=-W -Wall 

SRCS                :=  $(shell find $(SRC_DIR) -name '*.cpp')
SRCS                :=  $(sort $(SRCS))

INCLUDE_DIR         :=  $(ROOT_DIR)/include
INCLUDES            :=  $(shell find $(INCLUDE_DIR) -name '*.h*')
INCLUDES            +=  $(shell find $(INCLUDE_DIR) -name '*.hpp')
INCLUDE_DIR         :=  $(sort $(dir $(INCLUDES)))
INCLUDE_PATH       :=  $(foreach dir,$(INCLUDE_DIR), -I$(dir))

OBJS                :=  $(SRCS:%.cpp=%.o)
#OBJ_DIR             :=  $(sort $(dir $(OBJS)))
LIB_INCLUDE_PATH     += $(INCLUDE_PATH)
COMPILE         =  $(GPULS)  $(CXX_CFLAGS) -c "$<" -o "$@"$(LIB_INCLUDE_PATH) 
LIB_OBJECT = libmp4.a
.PHONY:all
all:$(LIB_OBJECT)
$(LIB_OBJECT):$(OBJS)
#	$(GPULS) $(OBJS) $(SHARED) -fPIC -o $(LIB_OBJECT)
	$(AR)	 rcs $(LIB_OBJECT)	$(OBJS)
	cp $(LIB_OBJECT) ../../$(LIBDIR)
#	cp $(LIB_OBJECT) $(TOP_DIR)/soc/mpp/lib
cp:all
	@mkdir -p $(LIBSRC_OUTPATH)/inc/
	cp $(LIB_OBJECT)	$(LIBSRC_OUTPATH)
	cp -rf $(ROOT_DIR)/include/ $(LIBSRC_OUTPATH)/inc/
$(OBJS) :%.o : %.cpp
	@echo complie $@
	$(COMPILE)
clean:
	$(RM) $(LIB_DIR)/$(LIB_OBJECT) $(OBJS) 
cleanall:clean
	#rm -rf 	$(LIBSRC_OUTPATH)/inc/