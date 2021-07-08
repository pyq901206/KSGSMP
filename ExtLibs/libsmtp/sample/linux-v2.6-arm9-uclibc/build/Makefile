.EXPORT_ALL_VARIABLES:
#VER = debug
VER = release

#BUILD_MOD = SHARED
BUILD_MOD = STATIC

TOPDIR = ../../../..

# for VCOS
VCOS_DEF = -D__VCOS_OS_LINUX -D__VCOS_GNUC -D__VCOS_LIB_UCLIBC
VCOS_DIR = $(TOPDIR)/VCOS/vcos_hilinux-uclibc_20070920
VCOS_INC = -I$(VCOS_DIR)/inc -I$(VCOS_DIR)/inc/sys -I$(VCOS_DIR)/inc/sysdeps/vcos_uclibc/i386  -I$(VCOS_DIR)/inc/sysdeps/generic \
           -I$(VCOS_DIR)/build
VCOS_LIB = $(VCOS_DIR)/subset-lib/subset3/static_library/libvcos_uclibc.a

# for MSG 
SMTP_DEF = 
SMTP_DIR = ../../..
SMTP_INC = -I$(SMTP_DIR)/include
ifeq ($(VER),debug)
	SMTP_LIB = $(SMTP_DIR)/lib/linux-v2.6-arm9-uclibc/debug/libsmtp.a
else
	SMTP_LIB = $(SMTP_DIR)/lib/linux-v2.6-arm9-uclibc/release/libsmtp.a
endif

# for XySsl
XYSSL_DIR = $(TOPDIR)/xyssl-0.8
XYSSL_INC = -I$(XYSSL_DIR)/include
XYSSL_LIB = $(XYSSL_DIR)/library/libxyssl.a

# for test
SMPL_BUILDDIR = ./
SMPL_SRCDIR = ../src
SMPL_BINDIR= ../bin


#CFLAGS= -Wall -I.. -D_REENTRANT -D__MSG_DEBUG__ $(CU_DEF) $(VCOS_DEF)
CFLAGS= -Wall -D_REENTRANT -D__MSG_DEBUG__ $(VCOS_DEF) $(SMTP_DEF)
ifeq ($(VER),debug)
  CFLAGS += -g3 -O0 
else 
  CFlAGS += -O3
endif


INCPATH	= $(VCOS_INC) $(SMTP_INC) 
LIBS = $(SMTP_LIB) $(XYSSL_LIB) $(VCOS_LIB) -lpthread -lrt -lm

CROSS_COMPILE 	=arm-linux-uclibc-

AS	=$(CROSS_COMPILE)as
LD	=$(CROSS_COMPILE)ld
CC	=$(CROSS_COMPILE)gcc
CPP	=$(CC) -E
AR	=$(CROSS_COMPILE)ar
NM	=$(CROSS_COMPILE)nm
STRIP	=$(CROSS_COMPILE)strip
OBJCOPY	=$(CROSS_COMPILE)objcopy
OBJDUMP	=$(CROSS_COMPILE)objdump


#PROC_OBJS = msg_config.o msgproc.o 
PROC_OBJS = 
SMPL_OBJS = sample_smtp.o

all: $(SMPL_OBJS) sample cleanobj
#all: $(PROC_OBJS) $(SMPL_OBJS) proc sample

sample_smtp.o: $(SMPL_SRCDIR)/sample_smtp.c
	@echo ========================================================
	@echo making: "$<" 
	@$(CC) $(SMPL_SRCDIR)/sample_smtp.c -c -o sample_smtp.o $(INCPATH) $(CFLAGS) $(EXTRA_CFLAGS) $(CFLAGS_$@) 
	@echo ========================================================
	@echo	

sample: $(SMPL_OBJS)
	@$(CC) -o $(SMPL_BINDIR)/sample_smtp -static $(SMPL_OBJS) $(LIBS)
	@echo "*** sample_smtp created OK! ***"; echo

clean:	
	@rm -f $(SMPL_BINDIR)/sample_smtp
	@rm -f *.o
	
cleanobj:
	@rm -f *.o
