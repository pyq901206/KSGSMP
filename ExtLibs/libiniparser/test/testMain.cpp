#include "../HI_IniPareser.h"

CHI_IniPareser * ini = NULL ; 

void testLoadFile(char* FilePath) ;
void testAddentry(char* FilePath,char* sec) ;
void testRead(char* FilePath,char* sec) ;
void testSetstr(char* FilePath,char* sec) ;
void testdump(char* FilePath);
void catstr(char**dst,char* src1,char* src2);
void testunset(char* filePath,char* sec,char*entry) ;
void testNULLPiont(char* filePath) ;
void main(int argc,char* argv[])
{

for (int i=atoi(argv[3]); i>0 ;i--)
{

	
	testLoadFile(argv[1]) ;
	testRead(argv[1],"test") ;
	testAddentry(argv[1],"add") ;
	testRead(argv[1],"add") ;
	testNULLPiont(argv[1]) ;

	testSetstr(argv[1],"test") ;
	testRead(argv[1],"test") ;
	char* dumpFile = argv[2];
	if (dumpFile== NULL)
	{
		dumpFile = "testdump.ini" ;
	}
//	testdump(dumpFile) ;
	testunset(argv[1],"add","test") ;
//	testLoadFile(dumpFile) ;
	testdump(dumpFile) ;

	testRead(argv[1],"test") ;
//	testdump(argv[4]) ;

	}

}

void testLoadFile(char* FilePath)
{
	HRESULT hr = 0;
	ini = new CHI_IniPareser();
	hr = ini->loadFile(FilePath);
	if (hr != 0)
	{
		printf("Can not Open File: %s  \r\n",FilePath) ;
		return ;
	}
	printf(" Open File: %s Successful! \r\n",FilePath) ;
}

void testAddentry(char* FilePath,char* sec)
{

	HRESULT hr = 0;
	if (ini == NULL)
	{
		ini = new CHI_IniPareser() ;
		ini->loadFile(FilePath) ;
	}
	char* val = NULL;
	char* argvstr = NULL;

	if (ini->find_entry(sec) != 0)
	{
		hr = ini->add_entry(sec,argvstr,val) ;
		printf("Hr = %d, add_entry(sec='%s',key =%s,val='%s') \r\n" ,hr,sec,argvstr,val);

	}

	  val ="set teststr1";
	  argvstr = "teststr" ;

	hr = ini->add_entry(sec,argvstr,val) ;
	printf("Hr = %d, add_entry(sec='%s',key =%s,val='%s') \r\n" ,hr,sec,argvstr,val);
	val = NULL ;

	val ="123546";
    argvstr = "testint" ;

	hr = ini->add_entry(sec,argvstr,val) ;
	printf("Hr = %d, add_entry(sec='%s',key =%s,val='%s') \r\n" ,hr,sec,argvstr,val);
	val = NULL ;

	val ="0.123456";
    argvstr = "testdouble" ;

	hr = ini->add_entry(sec,argvstr,val) ;
	printf("Hr = %d, add_entry(sec='%s',key =%s,val='%s') \r\n" ,hr,sec,argvstr,val);
	val = NULL ;
	val ="N";
    argvstr = "testboolean" ;

	hr = ini->add_entry(sec,argvstr,val) ;
	printf("Hr = %d, add_entry(sec='%s',key =%s,val='%s') \r\n" ,hr,sec,argvstr,val);
	val = NULL ;

	val ="t";
    argvstr = "boolean1" ;

	hr = ini->add_entry(sec,argvstr,val) ;
	printf("Hr = %d, add_entry(sec='%s',key =%s,val='%s') \r\n" ,hr,sec,argvstr,val);
	val = NULL ;


}

void testRead(char* FilePath,char* sec)
{
	if (ini == NULL)
	{
		ini = new CHI_IniPareser() ;
		ini->loadFile(FilePath) ;
	}
	char *argvstr=(char*)malloc(100);
	argvstr[0] = '\0';
	printf("------------------------read value from ini file------------------------\r\n");
	HRESULT hr=0 ;
	
	char * results = NULL;
	char*  key = ":teststr" ;
	catstr(&argvstr,sec,key);
	hr = ini->getstring(argvstr,&results) ;
	printf("Hr = %d, getstring(%s) result = %s \r\n" ,hr,argvstr,results);
	results = NULL ;

	key = ":nongstr";
	catstr(&argvstr,sec,key);

	hr = ini->getstring(argvstr,&results);
	printf("Hr = %d, getstring(%s) result = %s \r\n" ,hr,argvstr,results);
	results = NULL ;

	key = ":nongstr";
	catstr(&argvstr,sec,key);

	hr = ini->getstr(argvstr,&results);
	printf("Hr = %d, getstr(%s) result = %s \r\n" ,hr,argvstr,results);
	results = NULL ;

	key = ":testint";
	catstr(&argvstr,sec,key);

	int ret = 0;
	hr = ini->getint(argvstr,ret);
	printf("Hr = %d, getint(%s) result = %d \r\n",hr,argvstr,ret);
	results = NULL ;

	key = ":nongint";
	catstr(&argvstr,sec,key);

	ret = 0;
	hr = ini->getint(argvstr,ret);
	printf("Hr = %d, getint(%s) result = %d \r\n",hr,argvstr,ret);

	key = ":testdouble";
	catstr(&argvstr,sec,key);

	double retdb = 0.0;
	hr = ini->getdouble(argvstr,retdb);
	printf("Hr = %d, getint(%s) result = %f \r\n",hr,argvstr,retdb);

	key = ":nongdouble";
	catstr(&argvstr,sec,key);

	retdb = 0.0;
	hr = ini->getdouble(argvstr,retdb);
	printf("Hr = %d, getint(%s) result = %f \r\n",hr,argvstr,retdb);

	key = ":testboolean";
	catstr(&argvstr,sec,key);

	BOOL retB = FALSE;
	hr = ini->getboolean(argvstr,retB);
	printf("Hr = %d, getboolean(%s) result = %d \r\n",hr,argvstr,retB);

	key = ":testboolean1";
	catstr(&argvstr,sec,key);
	 retB = FALSE;
	hr = ini->getboolean(argvstr,retB);
	printf("Hr = %d, getboolean(%s) result = %d \r\n",hr,argvstr,retB);

	key = ":nongboolean";
	catstr(&argvstr,sec,key);
	retB = FALSE;
	hr = ini->getboolean(argvstr,retB);
	printf("Hr = %d, getboolean(%s) result = %d \r\n",hr,argvstr,retB);
	printf("------------------end  read value from ini file------------------------\r\n\r\n");
	free(argvstr);

}

void testSetstr(char* FilePath,char* sec)
{
	if (ini == NULL)
	{
		ini = new CHI_IniPareser() ;
		ini->loadFile(FilePath) ;
	}

	char *argvstr=(char*)malloc(100);
	argvstr[0] = '\0';


	HRESULT hr=0 ;
	char * val ="set t;;eststr1;;[dfgdfgdfgdf];;;";
	char*  key = ":teststr" ;
	catstr(&argvstr,sec,key);

	hr = ini->setstr(argvstr,val) ;
	printf("Hr = %d, setstr(entry='%s',val='%s') \r\n" ,hr,sec,val);
	val = NULL ;

	val ="set teststr1 nongstr";
	key = ":nongstr" ;
	catstr(&argvstr,sec,key);

	hr = ini->setstr(argvstr,val) ;
	printf("Hr = %d, setstr(entry='%s',val='%s') \r\n" ,hr,argvstr,val);
	val = NULL ;

	val ="20000";
	key = ":testint" ;
	catstr(&argvstr,sec,key);

	hr = ini->setstr(argvstr,val) ;
	printf("Hr = %d, setstr(entry='%s',val='%s') \r\n" ,hr,argvstr,val);
	val = NULL ;

	val ="0.100001";
	key = ":testdouble" ;
	catstr(&argvstr,sec,key);

	hr = ini->setstr(argvstr,val) ;
	printf("Hr = %d, setstr(entry='%s',val='%s') \r\n" ,hr,argvstr,val);
	val = NULL ;

	val ="N0000";
	key = ":testboolean" ;
	catstr(&argvstr,sec,key);

	hr = ini->setstr(argvstr,val) ;
	printf("Hr = %d, setstr(entry='%s',val='%s') \r\n" ,hr,argvstr,val);
	val = NULL ;

	val ="T0000";
	key = ":testboolean1" ;
	catstr(&argvstr,sec,key);

	hr = ini->setstr(argvstr,val) ;
	printf("Hr = %d, setstr(entry='%s',val='%s') \r\n" ,hr,argvstr,val);
	val = NULL ;
	free(argvstr);

}

void testdump(char* FilePath)
{
	HRESULT hr =0 ;

// 	hr = ini->dump_ini(NULL) ;
// 	printf("Hr =%d,dump_ini(NULL) .\r\n",hr) ;

	hr = ini->dump_ini(FilePath) ;
	printf("Hr =%d,dump_ini('%s') .\r\n",hr,FilePath) ;

	hr = ini->freedict() ;
	printf("Hr =%d,freedict() .\r\n",hr) ;
	
	hr = ini->freedict() ;
	printf("Hr =%d,freedict() twice .\r\n",hr) ;

	delete ini ;
	ini = NULL ;
}

void catstr(char**dst1,char* src1,char* src2)
{
	*dst1[0] = '\0';
	strcat(*dst1,src1);
	strcat(*dst1,src2);
}

void testunset(char* filePath,char* sec,char*entry)
{
	if (ini == NULL)
	{
		ini = new CHI_IniPareser() ;
		ini->loadFile(filePath) ;
	}
	HRESULT hr = 0;
	if (entry != NULL)
	{
		hr = ini->unset(entry) ;
		printf("hr= %d,unset('%s'). \r\n",hr,entry) ;
		return ;
	}
	 
	char* argvstr = (char*)malloc(100) ;
	argvstr[0] = '\0' ;
	
	char* key = ":teststr" ;
	catstr(&argvstr,sec,key);
    hr= ini->unset(argvstr);
	printf("hr=%d,unset('%s').\r\n",hr,argvstr) ;

	key = ":nonkey" ;
	catstr(&argvstr,sec,key);
    hr= ini->unset(argvstr);
	printf("hr=%d,unset('%s').\r\n",hr,argvstr) ;

	key = ":testdouble" ;
	catstr(&argvstr,sec,key);
    hr= ini->unset(argvstr);
	printf("hr=%d,unset('%s').\r\n",hr,argvstr) ;

	key = ":testint" ;
	catstr(&argvstr,sec,key);
    hr= ini->unset(argvstr);
	printf("hr=%d,unset('%s').\r\n",hr,argvstr) ;

	key = ":testboolean" ;
	catstr(&argvstr,sec,key);
    hr= ini->unset(argvstr);
	printf("hr=%d,unset('%s').\r\n",hr,argvstr) ;

	key = ":testboolean1" ;
	catstr(&argvstr,sec,key);
    hr= ini->unset(argvstr);
	printf("hr=%d,unset('%s'). \r\n",hr,argvstr) ;

	free(argvstr);
}

void testNULLPiont(char* filePath)
{
	HRESULT hr = 0;
	char* sec = NULL ;
	char* results = NULL ;
	char* val  = NULL ;
	char* key = NULL ;
	char* name = NULL ;
	
	if (ini == NULL)
	{
		ini = new CHI_IniPareser() ;
		ini->loadFile(filePath) ;
	}

	hr = ini->add_entry( sec,  key,  val);
	printf("hr= %d,add_entry(sec='%s',key='%s',val='%s') \r\n",hr,sec,key,val) ;
	
	key = "abc";
	hr = ini->add_entry( sec,  key,  val);
	printf("hr= %d,add_entry(sec='%s',key='%s',val='%s') \r\n",hr,sec,key,val) ;

	key = NULL;
	val = "123" ;
	hr = ini->add_entry( sec,  key,  val);
	printf("hr= %d,add_entry(sec='%s',key='%s',val='%s') \r\n",hr,sec,key,val) ;

	key = "abc";
	val = "123" ;
	hr = ini->add_entry( sec,  key,  val);
	printf("hr= %d,add_entry(sec='%s',key='%s',val='%s') \r\n",hr,sec,key,val) ;


	int n = 0;	

	hr = ini->getnsec(n);
	printf("hr= %d,getnsec() return n=%d \r\n",hr,n) ;


	hr = ini->getsecname(n, NULL);
	printf("hr= %d,>getsecname(n=%d, Name=%d) \r\n",hr,n,NULL) ;

	hr = ini->getsecname(n, &name);
	printf("hr= %d,>getsecname(n=%d, Name=%d) retNmae=%s \r\n",hr,n,NULL,name) ;
	
	hr = ini->getsecname(-1, &name);
	printf("hr= %d,>getsecname(n=%d, Name=%d) retNmae=%s \r\n",hr,-1,NULL,name) ;

	hr = ini->getsecname(1, &name);
	printf("hr= %d,>getsecname(n=%d, Name=%d) retNmae=%s \r\n",hr,1,&name,name) ;

/*
	hr = ini->dump_ini( char* pFileNmae);
	hr = ini->dump( char* pFileNmae);
	hr = ini->getstr( char * key,char** ppStrReturn);
	hr = ini->getstring( char * key, char** ppStrReturn);
	hr = ini->getint( char * key,int& retValue);
	hr = ini->getdouble( char * key, double& retValue);
	hr = ini->getboolean( char * key, BOOL& returnValue);
	hr = ini->setstr( char * entry,  char * val);
	hr = ini->unset( char * entry);
	hr = ini->find_entry( char * entry);
	hr = ini->loadFile( char * pininame);
	hr = ini->freedict();
*/

}

