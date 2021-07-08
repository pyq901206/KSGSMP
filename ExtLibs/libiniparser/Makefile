
#include $(shell pwd)/../../compiler.mk
include $(shell pwd)/../../config.local

CFLAGS  += -g -Wall

LIBPATH = ./lib
OBJPATH = ./obj

RANLIB = true

RM      = rm -f

INC := -I./include

#LIBSRC_OUTPATH = ../../libs

SUFFIXES = .o .c .h .a .so .sl

OUT_ROOT = $(OBJPATH)

all : $(OUT_ROOT) libiniparser.so  libiniparser.a

cp:all
	@mkdir -p $(LIBSRC_OUTPATH)/inc/
	@cp -rf ./include/*.h $(LIBSRC_OUTPATH)/inc/
	@cp -rf $(LIBPATH)/*.* $(LIBSRC_OUTPATH)


$(OUT_ROOT):
	mkdir -p "$(OUT_ROOT)"
	mkdir -p "$(LIBPATH)"
iniexample :
	@(cd test ; $(MAKE) ; cd ..)

COMPILE.c=$(CC) $(CFLAGS) 

$(OBJPATH)/%.o: ./src/%.c
	@(echo "compiling $< ...")
	($(COMPILE.c) -DHI_OS_LINUX -DHI_OS_TYPE=HI_OS_LINUX -o $@ -c $< $(INC))

SRCS = $(wildcard src/*.c)

OBJS = $(SRCS:%.c=%.o)
OBJS := $(OBJS:src/%=obj/%)

default:	libiniparser.so libiniparser.a

libiniparser.a:	$(OBJS)
	@echo "SRCS = " $(SRCS)
	@echo "OBJS = " $(OBJS)
	@($(AR) $(ARFLAGS) $(LIBPATH)/libiniparser.a $(OBJS))
	@($(RANLIB) $(LIBPATH)/libiniparser.a)
	@cp -rf $(LIBPATH)/*.a* ../../$(LIBDIR)
	
libiniparser.so:	$(OBJS)	
	@$(CC)  -fPIC -shared -o $(LIBPATH)/libiniparser.so $(OBJS)

clean:
	$(RM) $(OBJS)
	@rm -rf $(OBJPATH)

cleanall: clean
	$(RM) $(OBJS) $(LIBPATH)/libiniparser.a ./src/*.o
	rm -rf ./html ; # mkdir html
	@cd test ; $(MAKE) veryclean
	@rm -rf $(LIBSRC_OUTPATH)/libiniparser.a $(LIBSRC_OUTPATH)/libiniparser.so
	
docs:
	@(cd doc ; $(MAKE) ; cd ..)
	
check:
	@(cd test ; $(MAKE) ; cd ..)
