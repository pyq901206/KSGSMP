ifeq ($(TARGET),)
$(warn Not spec target, use arm-hismall-linux)
CROSS=arm-hismall-linux-
endif

MAKE	=$(CROSS)make
CC  	= $(CROSS)gcc
AR	    = $(CROSS)ar
#RANLIB = $(CROSS)ranlib
# Compiler settings
#CC      = gcc
CFLAGS  = -g -I../include

# Ar settings to build the library
#AR	    = ar
ARFLAGS = rcv

# Set RANLIB to ranlib on systems that require it (Sun OS < 4, Mac OSX)
# RANLIB  = ranlib
RANLIB = true

RM      = rm -f

# iniparser tests Makefile
#
LIBPATH = ../lib
EXEPATH = ../bin

#CC      = gcc
LFLAGS  = -L$(LIBPATH) -liniparser


default: all

all: iniexample

iniexample: iniexample.c
	@(echo "compiling iniexample.c ...")
	@($(CC) $(CFLAGS) -o $(EXEPATH)/iniexample iniexample.c $(LFLAGS) )

clean veryclean:
	$(RM) $(EXEPATH)/iniexample $(EXEPATH)/example.ini iniexample example.ini



