####################################################################################
#   makefile
####################################################################################

include ../../config.local

LIB   :=Libwipn.a
SRC += WiPN_API.c
SRC += WiPN_StringEncDec.c
SRC += wipnpush.c

export DEBUG_MODE = 1
#CFLAGS+=-DDEBUG
ifeq ($(DEBUG_MODE),1)
CFLAGS+= -DDEBUG_ON=1
endif
OBJS = $(addsuffix .o,$(basename $(SRC)))

all:$(LIB)
	
$(LIB) :$(OBJS)
	$(AR) -rc $(LIB) $(OBJS)
	$(CP) $(LIB) ../../$(LIBDIR)/
%.o : %.c
	$(CC)  $(BASE_CFLAGS) $(CFLAGS) -g  -c -I../../$(INCLUDEDIR)  $< -o $@

clean: 
	-rm -rf *.o
	-rm -rf $(LIB)
	-rm -rf ../../$(LIBDIR)/$(LIB)
