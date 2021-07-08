.EXPORT_ALL_VARIABLES:
VER = debug

#BUILD_MOD = SHARED
BUILD_MOD = STATIC

TOPDIR = ../../../../../

# for CUNIT
CU_DEF = 
CU_DIR  = /usr/local/sbin/
CU_INC  = -I$(CU_DIR)/include/CUnit
CU_LIB  = $(CU_DIR)/lib/libcunit.a

# for VCOS
VCOS_DEF = -D__VCOS_OS_LINUX -D__VCOS_GNUC -D__VCOS_LIB_UCLIBC
VCOS_DIR = $(TOPDIR)/VCOS/vcos_hilinux-uclibc_20070920
VCOS_INC = -I$(VCOS_DIR)/inc -I$(VCOS_DIR)/inc/sys -I$(VCOS_DIR)/inc/sysdeps/vcos_uclibc/i386  -I$(VCOS_DIR)/inc/sysdeps/generic \
           -I$(VCOS_DIR)/build
VCOS_LIB = $(VCOS_DIR)/subset-lib/subset3/static_library/libvcos_uclibc.a

# for MSG 
MSG_DEF = 
MSG_DIR = $(TOPDIR)/MSG
MSG_INC = -I$(MSG_DIR)/include -I$(MSG_DIR)/config
MSG_INN_INC = -I$(MSG_DIR)/src/include -I$(MSG_DIR)/src/source
MSG_SRCDIR = $(CODEDIR)/MSG/src/source
ifeq ($(VER),debug)
	MSG_LIB = $(MSG_DIR)/lib/linux-v2.6-arm9-uclibc/debug/libMSG.a
else
	MSG_LIB = $(MSG_DIR)/lib/linux-v2.6-arm9-uclibc/release/libMSG.a
endif

# for test
TEST_BUILDDIR = ./
TEST_SRCDIR = ../src
TEST_BINDIR= ../bin



#CFLAGS= -Wall -I.. -D_REENTRANT -D__MSG_DEBUG__ $(CU_DEF) $(VCOS_DEF)
CFLAGS= -I.. -D_REENTRANT -D__MSG_DEBUG__ $(CU_DEF) $(VCOS_DEF)
ifeq ($(VER),debug)
  CFLAGS += -g3 -O0 
else 
  CFlAGS += -O3
endif


INCPATH	= $(CU_INC) $(VCOS_INC) $(MSG_INC) $(MSG_INN_INC)
LIBS = $(CU_LIB) $(MSG_LIB) $(VCOS_LIB) -lpthread -lrt -lm

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


TEST_OBJS = msg_config.o ut_main.o ut_tab_misc.o ut_mb.o ut_mq.o ut_socket.o ut_priorq.o ut_connq.o ut_com.o ut_misc.o

msg_config.o: $(MSG_DIR)/config/msg_config.c
	$(CC) $(MSG_DIR)/config/msg_config.c -c -o msg_config.o $(INCPATH) $(CFLAGS) $(EXTRA_CFLAGS) $(CFLAGS_$@)
 
ut_main.o: $(TEST_SRCDIR)/ut_main.c
	$(CC) $(TEST_SRCDIR)/ut_main.c -c -o ut_main.o $(INCPATH) $(CFLAGS) $(EXTRA_CFLAGS) $(CFLAGS_$@) 

ut_tab_misc.o: $(TEST_SRCDIR)/ut_tab_misc.c
	$(CC) $(TEST_SRCDIR)/ut_tab_misc.c -c -o ut_tab_misc.o $(INCPATH) $(CFLAGS) $(EXTRA_CFLAGS) $(CFLAGS_$@) 

ut_mb.o: $(TEST_SRCDIR)/ut_mb.c
	$(CC) $(TEST_SRCDIR)/ut_mb.c -c -o ut_mb.o $(INCPATH) $(CFLAGS) $(EXTRA_CFLAGS) $(CFLAGS_$@)

ut_mq.o: $(TEST_SRCDIR)/ut_mq.c
	$(CC) $(TEST_SRCDIR)/ut_mq.c -c -o ut_mq.o $(INCPATH) $(CFLAGS) $(EXTRA_CFLAGS) $(CFLAGS_$@)

ut_socket.o: $(TEST_SRCDIR)/ut_socket.c
	$(CC) $(TEST_SRCDIR)/ut_socket.c -c -o ut_socket.o $(INCPATH) $(CFLAGS) $(EXTRA_CFLAGS) $(CFLAGS_$@)

ut_priorq.o: $(TEST_SRCDIR)/ut_priorq.c
	$(CC) $(TEST_SRCDIR)/ut_priorq.c -c -o ut_priorq.o $(INCPATH) $(CFLAGS) $(EXTRA_CFLAGS) $(CFLAGS_$@)

ut_connq.o: $(TEST_SRCDIR)/ut_connq.c
	$(CC) $(TEST_SRCDIR)/ut_connq.c -c -o ut_connq.o $(INCPATH) $(CFLAGS) $(EXTRA_CFLAGS) $(CFLAGS_$@)

ut_com.o: $(TEST_SRCDIR)/ut_com.c
	$(CC) $(TEST_SRCDIR)/ut_com.c -c -o ut_com.o $(INCPATH) $(CFLAGS) $(EXTRA_CFLAGS) $(CFLAGS_$@)

ut_misc.o: $(TEST_SRCDIR)/ut_misc.c
	$(CC) $(TEST_SRCDIR)/ut_misc.c -c -o ut_misc.o $(INCPATH) $(CFLAGS) $(EXTRA_CFLAGS) $(CFLAGS_$@)

all: $(TEST_OBJS)
	$(CC) -o $(TEST_BINDIR)/ut -static $(TEST_OBJS) $(LIBS)

clean:	
	rm -f *.lib
	rm -f *.a
	rm -f *.so
	rm -f *.o
	

