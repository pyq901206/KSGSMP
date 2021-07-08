####################################################################################
#   makefile
####################################################################################

include ../../config.local

LIB   :=LibhkManage.a
SRC += hkManage.c
SRC += hklistconf.c
SRC += hkDataprase.c

INCLUDE += ./incCn
OBJS = $(addsuffix .o,$(basename $(SRC)))

all:$(LIB)
	
$(LIB) :$(OBJS)
	$(AR) -rc $(LIB) $(OBJS)
	$(CP) $(LIB) ../../$(LIBDIR)/
%.o : %.c
	$(CPP)  $(BASE_CFLAGS) -g  -c -I../../$(INCLUDEDIR) -I$(INCLUDE) -I./ $< -o $@

clean: 
	-rm -rf *.o
	-rm -rf $(LIB)
	-rm -rf ../../$(LIBDIR)/$(LIB)
