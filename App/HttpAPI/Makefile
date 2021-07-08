####################################################################################
#   makefile
####################################################################################

include ../../config.local

LIB   :=LibHttpApi.a
SRC += httpapi.c
SRC += httppares.c
INCLUDE += ../include
OBJS = $(addsuffix .o,$(basename $(SRC)))

all:$(LIB)
	
$(LIB) :$(OBJS)
	$(AR) -rc $(LIB) $(OBJS)
	$(CP) $(LIB) ../../$(LIBDIR)/
%.o : %.c
	$(CC)  $(BASE_CFLAGS) -g  -c -I../../$(INCLUDEDIR) -I../../$(INCLUDEDIR)/curl -I$(INCLUDE) $< -o $@

clean: 
	-rm -rf *.o
	-rm -rf $(LIB)
	-rm -rf ../../$(LIBDIR)/$(LIB)
