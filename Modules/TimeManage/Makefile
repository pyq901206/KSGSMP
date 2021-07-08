####################################################################################
#   makefile
####################################################################################

include ../../config.local

LIB   :=LibTimeManage.a
SRC += timemanage.c
SRC += timemanageconfig.c

OBJS = $(addsuffix .o,$(basename $(SRC)))

all:$(LIB)
	
$(LIB) :$(OBJS)
	$(AR) -rc $(LIB) $(OBJS)
	$(CP) $(LIB) ../../$(LIBDIR)/
%.o : %.c
	$(CC)  $(BASE_CFLAGS) -g  -c -I../../$(INCLUDEDIR) $< -o $@

clean: 
	-rm -rf *.o
	-rm -rf $(LIB)
	-rm -rf ../../$(LIBDIR)/$(LIB)
