typedef struct _FTP_T{
	char 	usrname[32];
	char 	password[32];
	char	ipAddr[16];	/*ftp 服务器*/
	int 	port;		/*ftp端口*/
	ENUM_ENABLED_E 	enabled;	/*是否启动ftp上传功能*/
}FTP_T;z