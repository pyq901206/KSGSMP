export ROOT_DIR:=$(shell pwd)

TOP_DIR := ..
include ../../config.local

SRC_DIR += $(ROOT_DIR)/core/src \
			$(ROOT_DIR)/imgproc/src



RM = rm -rf

#****************************************************************************


CXX_DEFINES = -DCVAPI_EXPORTS
CXX_CFLAGS+=-fsigned-char -W -Wall -Werror=return-type -Werror=address -Werror=sequence-point -Wformat -Werror=format-security -Wmissing-declarations -Wundef -Winit-self -Wpointer-arith -Wshadow -Wsign-promo -fdiagnostics-show-option -pthread -fomit-frame-pointer -ffunction-sections -O2 -DNDEBUG  -DNDEBUG -fPIC

SRCS                :=  $(shell find $(SRC_DIR) -name '*.cpp')
SRCS                :=  $(sort $(SRCS))

INCLUDES_DIR         :=  ../../$(INCLUDEDIR)/opencv
INCLUDES            :=  $(shell find $(INCLUDES_DIR) -name '*.h*')
INCLUDES            +=  $(shell find $(INCLUDES_DIR) -name '*.hpp')
INCLUDES_DIR         :=  $(sort $(dir $(INCLUDES)))
INCLUDE_PATH       :=  $(foreach dir,$(INCLUDES_DIR), -I$(dir))

OBJS                :=  $(SRCS:%.cpp=%.o)
#OBJ_DIR             :=  $(sort $(dir $(OBJS)))
LIB_INCLUDE_PATH     += $(INCLUDE_PATH)
COMPILE         =  $(CPP) $(CXX_DEFINES) $(CXX_CFLAGS) -c "$<" -o "$@"$(LIB_INCLUDE_PATH) 
LIB_OBJECT = libopencv_core.a


.PHONY:all

all:$(LIB_OBJECT)

$(LIB_OBJECT):$(OBJS)

#	$(GPULS) $(OBJS) $(SHARED) -fPIC -o $(LIB_OBJECT)
	$(AR)	 rcs $(LIB_OBJECT)	$(OBJS)
	$(CP) $(LIB_OBJECT) ../../$(LIBDIR)/
$(OBJS) :%.o : %.cpp
	@echo complie $@
	$(COMPILE)

clean:
	$(RM) ../../$(LIB_DIR)/$(LIB_OBJECT) $(OBJS) 
