####################################################################################
#   makefile
####################################################################################

include ../../config.local

LIB   :=LibHumanDataAnalysis.a
SRC += humandataAnalysis.c

OBJS = $(addsuffix .o,$(basename $(SRC)))

all:$(LIB)
	
$(LIB) :$(OBJS)
	$(AR) -rc $(LIB) $(OBJS)
	$(CP) $(LIB) ../../$(LIBDIR)/
%.o : %.c
	$(CC)  $(BASE_CFLAGS) -g  -c -I../../$(INCLUDEDIR)/ -I../../$(INCLUDEDIR)/curl -I../../$(INCLUDEDIR)/polarssl $< -o $@

clean: 
	-rm -rf *.o
	-rm -rf $(LIB)
	-rm -rf ../../$(LIBDIR)/$(LIB)
