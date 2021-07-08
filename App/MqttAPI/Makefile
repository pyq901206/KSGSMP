####################################################################################
#   makefile
####################################################################################

include ../../config.local

LIB   :=LibMqttApi.a
SRC += mqttapi.c

INCLUDE += ../include
OBJS = $(addsuffix .o,$(basename $(SRC)))

all:$(LIB)
	
$(LIB) :$(OBJS)
	$(AR) -rc $(LIB) $(OBJS)
	$(CP) $(LIB) ../../$(LIBDIR)/
%.o : %.c
	$(CC)  $(BASE_CFLAGS) -g  -c -I../../$(INCLUDEDIR) -I$(INCLUDE) $< -o $@

clean: 
	-rm -rf *.o
	-rm -rf $(LIB)
	-rm -rf ../../$(LIBDIR)/$(LIB)
