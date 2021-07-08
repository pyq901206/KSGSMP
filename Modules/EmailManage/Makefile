####################################################################################
#   makefile
####################################################################################

include ../../config.local

LIB   :=LibEmailManage.a
SRC += emailmanage.c
SRC += emailconfig.c

OBJS = $(addsuffix .o,$(basename $(SRC)))

all:$(LIB)
	
$(LIB) :$(OBJS)
	$(AR) -rc $(LIB) $(OBJS)
	$(CP) $(LIB) ../../$(LIBDIR)/
%.o : %.c
	$(CC)  $(BASE_CFLAGS) -g  -c -I../../$(INCLUDEDIR)/ -I../../$(INCLUDEDIR)/smtp -I../../$(INCLUDEDIR)/polarssl $< -o $@

clean: 
	-rm -rf *.o
	-rm -rf $(LIB)
	-rm -rf ../../$(LIBDIR)/$(LIB)
