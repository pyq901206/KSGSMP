#********************* Work directory **********************
.EXPORT_ALL_VARIABLES:

WORKDIR := $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)

#include $(WORKDIR)/../../compiler.mk
include $(WORKDIR)/../../config.local
#ifeq ($(CROSS),)
#$(warn Not spec target, use arm-hismall-linux)
#CROSS=arm-hismall-linux-
#CROSS=arm-hisiv100-linux-
#endif

ifeq ($(POLARSSL_VER),)
POLARSSL_VER = 1
endif
# for POLARSSL
ifeq ($(POLARSSL_VER),1)
POLARSSL_DIR = $(WORKDIR)/../polarssl-1.0.0/
POLARSSL_INC = -I$(POLARSSL_DIR)/include/
endif

# for SCP Module
SCP_NAME = SMTP
SCP_NAME_LIB = $(shell echo $(SCP_NAME) | tr 'A-Z' 'a-z')
SCP_DIR = $(WORKDIR)
SCP_DEF = 
SCP_INC = -I$(SCP_DIR)/include -I$(SCP_DIR)/config/ -I$(SCP_DIR)/src/include -I$(SCP_DIR)/source
SCP_SRCDIR = $(SCP_DIR)/src/source
SCP_LIB_DIR = $(SCP_DIR)/lib/

SCP_DEF_DEBUG += -D__SMTP_DEBUG__
ifeq ($(POLARSSL_VER),0)
SCP_DEF_DEBUG += -DCMP_POLARSSL=0
endif
ifeq ($(POLARSSL_VER),1)
SCP_DEF_DEBUG += -DCMP_POLARSSL=1
endif
SCP_LIB_DIR_DEBUG = $(SCP_LIB_DIR)/debug
SCP_OBJ_DIR_DEBUG = $(SCP_LIB_DIR_DEBUG)/obj
SCP_LIB_FILE_DEBUG_STATIC  = $(SCP_LIB_DIR_DEBUG)/lib$(SCP_NAME_LIB).a
SCP_LIB_FILE_DEBUG_DYNAMIC = $(SCP_LIB_DIR_DEBUG)/lib$(SCP_NAME_LIB).so

SCP_DEF_RELEASE += 
ifeq ($(POLARSSL_VER),0)
SCP_DEF_RELEASE += -DCMP_POLARSSL=0
endif
ifeq ($(POLARSSL_VER),1)
SCP_DEF_RELEASE += -DCMP_POLARSSL=1
endif
SCP_LIB_DIR_RELEASE = $(SCP_LIB_DIR)/release
SCP_OBJ_DIR_RELEASE = $(SCP_LIB_DIR_RELEASE)/obj
SCP_LIB_FILE_RELEASE_STATIC  = $(SCP_LIB_DIR_RELEASE)/lib$(SCP_NAME_LIB).a
SCP_LIB_FILE_RELEASE_DYNAMIC = $(SCP_LIB_DIR_RELEASE)/lib$(SCP_NAME_LIB).so


CFLAGS_DEBUG = -g3 -fPIC -O0 -Wall -I.. -D_REENTRANT  $(SCP_DEF_DEBUG)
CFLAGS_RELEASE  =  -fPIC -O3 -Wall -I.. -D_REENTRANT  $(SCP_DEF_RELEASE)

ifeq ($(POLARSSL_VER),1)
INCPATH	= $(POLARSSL_INC) $(SCP_INC) 
endif
ifeq ($(POLARSSL_VER),0)
INCPATH	= $(SCP_INC) 
endif
LIBS_SHARED = -lpthread -lrt -lm

#CROSS_COMPILE = arm-linux-uclibc-
#CROSS_COMPILE = arm-hisiv100-linux-

#AS	=$(CROSS)as
#LD	=$(CROSS)ld
#CC	=$(CROSS)gcc 
#CPP	=$(CC) -E
#AR	=$(CROSS)ar
#NM	=$(CROSS)nm
#STRIP	=$(CROSS)strip
#OBJCOPY	=$(CROSS_CROSS)objcopy
#OBJDUMP	=$(CROSS_CROSS)objdump


COMPILE_DEBUG   = @$(CC) "$<" -c -o "$(SCP_OBJ_DIR_DEBUG)/$(*F).o" $(CFLAGS_DEBUG) $(INCPATH) 
COMPILE_RELEASE = @$(CC) "$<" -c -o "$(SCP_OBJ_DIR_RELEASE)/$(*F).o" $(CFLAGS_RELEASE) $(INCPATH) 

#********************* OBJs Define **************************
SCP_SRC = $(wildcard $(SCP_SRCDIR)/*.c)
SCP_SRC_NOTDIR = $(notdir $(SCP_SRC))

SCP_OBJ_DEBUG   = $(addprefix $(SCP_OBJ_DIR_DEBUG)/, $(SCP_SRC_NOTDIR:%.c=%.o))
SCP_OBJ_RELEASE = $(addprefix $(SCP_OBJ_DIR_RELEASE)/, $(SCP_SRC_NOTDIR:%.c=%.o))

all: prepare \
     $(SCP_OBJ_DEBUG) $(SCP_LIB_FILE_DEBUG_STATIC) $(SCP_LIB_FILE_DEBUG_DYNAMIC) \
     $(SCP_OBJ_RELEASE) $(SCP_LIB_FILE_RELEASE_STATIC) $(SCP_LIB_FILE_RELEASE_DYNAMIC)  \
     cleanobj	

$(SCP_LIB_FILE_DEBUG_STATIC): $(SCP_OBJ)
	@$(AR) -r $(SCP_LIB_FILE_DEBUG_STATIC) $(SCP_OBJ_DEBUG)
	@echo "*** Static lib $(@F)  created OK! ***"; echo

$(SCP_LIB_FILE_DEBUG_DYNAMIC): $(SCP_OBJ)
	@$(CC) -shared -o $(SCP_LIB_FILE_DEBUG_DYNAMIC) $(SCP_OBJ_DEBUG)
	@echo "*** Static lib $(@F)  created OK! ***"; echo

$(SCP_LIB_FILE_RELEASE_STATIC): $(SCP_OBJ_RELEASE)
	@$(AR) -r $(SCP_LIB_FILE_RELEASE_STATIC) $(SCP_OBJ_RELEASE)
	@echo "*** Static lib $(@F)  created OK! ***"; echo

$(SCP_LIB_FILE_RELEASE_DYNAMIC): $(SCP_OBJ_RELEASE)
	@$(CC) -shared -o $(SCP_LIB_FILE_RELEASE_DYNAMIC) $(SCP_OBJ_RELEASE)
	@echo "*** Static lib $(@F)  created OK! ***"; echo

	#cp -rf ./include/*.h ../../$(INCLUDEDIR)/smtp/
	cp -rf $(SCP_LIB_DIR_RELEASE)/*.a ../../$(LIBDIR)
#********************* OBJs Define **************************
$(SCP_OBJ_DIR_DEBUG)/%.o : $(SCP_SRCDIR)/%.c
	@echo ========================================================
	@echo making: "$<" 
	$(COMPILE_DEBUG)
	@echo ========================================================
	@echo

	
$(SCP_OBJ_DIR_RELEASE)/%.o : $(SCP_SRCDIR)/%.c
	@echo ========================================================
	@echo making: "$<" 
	$(COMPILE_RELEASE)
	@echo ========================================================
	@echo


#********************* Create the dirs or files needed ******
prepare:
	@mkdir -p $(SCP_LIB_DIR_DEBUG)
	@mkdir -p $(SCP_LIB_DIR_RELEASE)
	@mkdir -p $(SCP_OBJ_DIR_DEBUG)
	@mkdir -p $(SCP_OBJ_DIR_RELEASE)
	
cp:all
#	@mkdir -p $(LIBSRC_OUTPATH)/inc/
#	@cp -rf ./include/*.h $(LIBSRC_OUTPATH)/inc/
#	@cp -arf $(SCP_LIB_DIR_RELEASE)/*.* $(LIBSRC_OUTPATH)
#	cp -rf ./include/*.h ../../$(INCLUDEDIR)/smtp/
#	cp -rf $(SCP_LIB_DIR_RELEASE)/*.a ../../$(LIBDIR)
cleanobj:
	@rm -rf $(SCP_OBJ_DIR_DEBUG)
	@rm -rf $(SCP_OBJ_DIR_RELEASE)


# Rebuild this project
rebuild: cleanall all

# Clean this project
clean:
	@rm -rf $(SCP_LIB_DIR)/*

# Clean this project and all dependencies
cleanall: clean
	@rm -rf ../../$(LIBDIR)/libsmtp*.*	



