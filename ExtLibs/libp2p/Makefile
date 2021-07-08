####################################################################################
#   makefile
####################################################################################

include ../../config.local

LIB   :=Libp2p.a
SRC += p2p_swap.c tcp_swap.c
OBJS = $(addsuffix .o,$(basename $(SRC)))
EXT_LIBS += ./PPCS/Lib/Linux/Hi3518/libPPCS_API.a
EXT_LIBS_INC += ./PPCS/Include ./PPCS/Include/PPCS
BUILD_FLAG += $(addprefix -I,$(EXT_LIBS_INC))
BUILD_FLAG += -DLINUX
all:$(LIB)
	
$(LIB) :$(OBJS)
	$(AR) -rc $(LIB)   $(OBJS)
	$(CP) $(LIB) ../../$(LIBDIR)/
	$(CP) $(EXT_LIBS) ../../$(LIBDIR)/
%.o : %.c
	$(CC)  $(BASE_CFLAGS) -g  -c -I../../$(INCLUDEDIR) $(BUILD_FLAG) $< -o $@

clean: 
	-rm -rf *.o
	-rm -rf $(LIB)
	-rm -rf ../../$(LIBDIR)/$(LIB)
