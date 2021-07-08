####################################################################################
#   makefile
####################################################################################
include ../config.local

#########################
LOCAL_TARGET = SmartPlat_main64

Need_Lib +=  ../$(LIBDIR)/LibAdpApi.a
Need_Lib +=  ../$(LIBDIR)/LibDeviceInfo.a
Need_Lib +=  ../$(LIBDIR)/LibDeviceManage.a
Need_Lib +=  ../$(LIBDIR)/LibPublickKey.a
Need_Lib +=  ../$(LIBDIR)/LibTimeManage.a
Need_Lib +=  ../$(LIBDIR)/LibMqttApi.a
Need_Lib +=  ../$(LIBDIR)/LibCICTApi.a
Need_Lib +=  ../$(LIBDIR)/LibHJ212Api.a
Need_Lib +=  ../$(LIBDIR)/LibHttpApi.a
Need_Lib +=  ../$(LIBDIR)/LibhkManage.a
Need_Lib +=  ../$(LIBDIR)/Libattendancemanage.a
Need_Lib +=  ../$(LIBDIR)/LibWebServer.a
Need_Lib +=  ../$(LIBDIR)/LibJsonApi.a
Need_Lib +=  ../$(LIBDIR)/LibConfigManage.a
Need_Lib +=  ../$(LIBDIR)/LibCJson.a
Need_Lib +=  ../$(LIBDIR)/LibUsrString.a
Need_Lib +=  ../$(LIBDIR)/LibUsrMem.a
Need_Lib +=  ../$(LIBDIR)/LibHandleManage.a
Need_Lib +=  ../$(LIBDIR)/LibSysManage.a
Need_Lib +=  ../$(LIBDIR)/LIBVehicleDataAnalysis.a
Need_Lib +=  ../$(LIBDIR)/LibHumanDataAnalysis.a
Need_Lib +=  ../$(LIBDIR)/LIBFuelQuantityAnalysis.a
Need_Lib +=  ../$(LIBDIR)/LibAlarmManage.a
Need_Lib +=  ../$(LIBDIR)/LibMediaQueue.a
Need_Lib +=  ../$(LIBDIR)/LibMessage.a
Need_Lib +=  ../$(LIBDIR)/librdkafka.a
Need_Lib +=  ../$(LIBDIR)/Libmysqlctrl.a
Need_Lib +=  ../$(LIBDIR)/LibMyAuthor.a
Need_Lib +=  ../$(LIBDIR)/LibRtmp.a
Need_Lib +=  ../$(LIBDIR)/libmtrtmp.a
Need_Lib +=  ../$(LIBDIR)/LibLog.a
Need_Lib +=  ../$(LIBDIR)/LibUsrManage.a
Need_Lib +=  ../$(LIBDIR)/LibBin2Hex.a
Need_Lib +=  ../$(LIBDIR)/libmosquitto.a
Need_Lib +=  ../$(LIBDIR)/libcurl.so
Need_Lib +=  ../$(LIBDIR)/LibTSocket.a
Need_Lib +=  ../$(LIBDIR)/LibEventSocket.a
Need_Lib +=  ../$(LIBDIR)/libevent.a
Need_Lib +=  ../$(LIBDIR)/LibList.a
Need_Lib +=  ../$(LIBDIR)/LibBase64.a
Need_Lib +=  ../$(LIBDIR)/libpolarssl.a
Need_Lib +=  ../$(LIBDIR)/LibJpegDel.a
Need_Lib +=  ../$(LIBDIR)/libjpeg.a
Need_Lib +=  ../$(LIBDIR)/libturbojpeg.a
Need_Lib +=  ../$(LIBDIR)/libpng.a
Need_Lib +=  ../$(LIBDIR)/libz.a
Need_Lib +=  ../$(LIBDIR)/libmxml.a
Need_Lib +=  ../$(LIBDIR)/libcurl.a

LIB_PATH=-L../$(LIBDIR)
LIB_NAME= -lHCCore -lhcnetsdk -lhpr

all: $(LOCAL_TARGET)
CCFLAGS= -D_DETACHED_THREAD_ -D__DEBUG__ -D__NODE_NAME__=\"$(LOCAL_TARGET)\"
TEST_OBJS += main.o 

$(LOCAL_TARGET): $(TEST_OBJS) 
	
	$(CC) $(WAR) $(BASE_CFLAGS) \
	$(TEST_OBJS) \
	$(Need_Lib) \
	 -lstdc++ -lnsl -lresolv  -ldl -lm -lpthread -lrt -lz -lcrypto -liconv -lmysqlclient $(LIB_PATH) $(LIB_NAME) -o $@

	$(STRIP) $(LOCAL_TARGET)
	rm -f *.o
	$(MV) $(LOCAL_TARGET) ../

%.o:%.c
	$(CC) $(WAR) $(CCFLAGS)  	-I../$(INCLUDEDIR)   $(MAKETARGET) -ldl  -c $<
	
	
clean: 
	rm -f *.o $(LOCAL_TARGET)
	rm -f  ../$(LOCAL_TARGET)
