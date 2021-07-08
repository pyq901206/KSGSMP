
DESTDIR=/usr/local
PREFIX=polarssl_

#ifeq ($(CROSS),)
#$(warn Not spec target, use arm-hismall-linux)
#CROSS=arm-hismall-linux-
#CROSS=arm-hisiv100-linux-
#endif


#export CC=$(CROSS)gcc
#export AR=$(CROSS)ar

#include $(shell pwd)/../../compiler.mk
include ../../config.local
#LIBSRC_OUTPATH = ../../libs
.SILENT:

all:
	cd library  && $(MAKE) all && cd ..
	#cd programs && $(MAKE) all && cd ..
	#cd tests    && $(MAKE) all && cd ..
	cp -rf ./include/polarssl/*.h ../../$(INCLUDEDIR)/polarssl/
	cp -rf ./library/*.a ../../$(LIBDIR)
#	$(MAKE) cp
cp:all
#	@mkdir -p $(LIBSRC_OUTPATH)/inc/
#	@cp -rf ./include/polarssl/ $(LIBSRC_OUTPATH)/inc/
#	@cp -rf ./library/*.a $(LIBSRC_OUTPATH)
	
install:
	mkdir -p $(DESTDIR)/include/polarssl
	cp -r include/polarssl/ssl.h $(DESTDIR)/include
	cp -r include/polarssl/net.h $(DESTDIR)/include
	cp -r include/polarssl/dhm.h $(DESTDIR)/include
	cp -r include/polarssl/rsa.h $(DESTDIR)/include
	cp -r include/polarssl/md5.h $(DESTDIR)/include
	cp -r include/polarssl/sha1.h $(DESTDIR)/include
	
	mkdir -p $(DESTDIR)/lib
	cp library/libpolarssl.* $(DESTDIR)/lib
	
	mkdir -p $(DESTDIR)/bin
	for p in programs/*/* ; do              \
	    if [ -x $$p ] && [ ! -d $$p ] ;     \
	    then                                \
	        f=$(PREFIX)`basename $$p` ;     \
	        cp $$p $(DESTDIR)/bin/$$f ;     \
	    fi                                  \
	done

clean:
	cd library  && $(MAKE) clean && cd ..
	#cd programs && $(MAKE) clean && cd ..
	#cd tests    && $(MAKE) clean && cd ..
	rm -rf ../../$(LIBDIR)/libpolarssl*.a

cleanall: clean
	@rm -rf $(LIBSRC_OUTPATH)/libpolarssl*.a	

check:
	( cd tests && $(MAKE) check )

apidoc:
	mkdir -p apidoc
	doxygen doxygen/polarssl.doxyfile

apidoc_clean:
	if [ -d apidoc ] ;			\
	then				    	\
		rm -rf apidoc ;			\
	fi
