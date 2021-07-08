####################################################################################
#   makefile
####################################################################################

include ../../config.local

LIB   :=LibRtmp.a
SRC += rtmpstart.c
SRC += rtmpconfig.c
SRC += rtmpjsonmap.c
SRC += coding.c

INCLUDE += ./
OBJS = $(addsuffix .o,$(basename $(SRC)))

all:$(LIB)
	
$(LIB) :$(OBJS)
	$(AR) -rc $(LIB) $(OBJS)
	$(CP) $(LIB) ../../$(LIBDIR)/
	$(CP) libaacenc.a ../../$(LIBDIR)/
%.o : %.c
	$(CC)  $(BASE_CFLAGS) -g  -c -I../../$(INCLUDEDIR) -I../../$(INCLUDEDIR)/rtmp -I$(INCLUDE) $< -o $@

clean: 
	-rm -rf *.o
	-rm -rf $(LIB)
	-rm -rf ../../$(LIBDIR)/libaacenc.a
	-rm -rf ../../$(LIBDIR)/$(LIB)
