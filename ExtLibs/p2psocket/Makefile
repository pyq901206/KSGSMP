####################################################################################
#   makefile
####################################################################################

include ../../config.local

LIB   :=LibP2Psocket.a
SRC += p2psocket.c

OBJS = $(addsuffix .o,$(basename $(SRC)))
EXT_LIBS_INC += ../libp2p/
BUILD_FLAG += $(addprefix -I,$(EXT_LIBS_INC))
BUILD_FLAG += -DLINUX
all:$(LIB)
	
$(LIB) :$(OBJS)
	$(AR) -rc $(LIB) $(OBJS)
	$(CP) $(LIB) ../../$(LIBDIR)/
%.o : %.c
	$(CC)  $(BASE_CFLAGS) -g  -c -I../../$(INCLUDEDIR) $(BUILD_FLAG) $< -o $@

clean: 
	-rm -rf *.o
	-rm -rf $(LIB)
	-rm -rf ../../$(LIBDIR)/$(LIB)
