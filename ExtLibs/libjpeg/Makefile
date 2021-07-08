####################################################################################
#   makefile
####################################################################################

include ../../config.local

LIB   :=LibJpegDel.a
SRC += jpeg_yuv.c

OBJS = $(addsuffix .o,$(basename $(SRC)))

all:$(LIB)
	
$(LIB) :$(OBJS)
	$(AR) -rc $(LIB) $(OBJS)
	$(CP) $(LIB) ../../$(LIBDIR)/
%.o : %.c
	$(CC)  $(BASE_CFLAGS) -g  -c -I../../$(INCLUDEDIR) -I../../$(INCLUDEDIR)/libjpeg-turbo $< -o $@

clean: 
	-rm -rf *.o
	-rm -rf $(LIB)
	-rm -rf ../../$(LIBDIR)/$(LIB)
