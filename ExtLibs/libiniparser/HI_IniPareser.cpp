/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : HI_IniPareser.cpp
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2006/12/29
  Description   : IniPareser interface
  History       :
  1.Date        : 20067/12/29
    Author      : guiliangping 60020692
    Modification: Created file

******************************************************************************/

#include "HI_IniPareser.h"
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHI_IniPareser::CHI_IniPareser()
{
	m_pFileName =NULL;
	m_pfile = NULL;
	m_pDictionary = NULL ;
	m_iniState = FALSE ;
}

CHI_IniPareser::~CHI_IniPareser()
{
	if (m_pfile != NULL)
	{
		free(m_pfile);
		m_pfile = NULL ;
	}
	if (m_pDictionary != NULL)
	{
		freedict() ;
		m_pDictionary = NULL;
	}
	if (m_pFileName !=NULL)
	{
		free(m_pFileName) ;
		m_pFileName = NULL ;
	}
}


HRESULT CHI_IniPareser::add_entry(char * sec, char * key, char * val)
{
	HRESULT hr = 0;  
	if (m_iniState == FALSE)
	{
		return HI_ERROR_INIP_NOTINI ;//not init m_pDictionary
	}

	if (sec == NULL)
	{
		return E_INVALIDARG ;
	}

	 hr= iniparser_add_entry(m_pDictionary,sec ,key ,val) ;

	return hr ;
}
HRESULT CHI_IniPareser::getnsec(int& number)
{
	HRESULT hr = 0; 
	if (m_iniState == FALSE)
	{
		return HI_ERROR_INIP_NOTINI ;//not init m_pDictionary
	}
	number = iniparser_getnsec(m_pDictionary) ;
	return hr ;
}
HRESULT CHI_IniPareser::getsecname(int n,char **Name)
{
	HRESULT hr = 0; 
	if (m_iniState == FALSE)
	{
		return HI_ERROR_INIP_NOTINI ;//not init m_pDictionary
	}
	if (Name == NULL||n<0)
	{
		return E_INVALIDARG;
	}
	*Name = iniparser_getsecname(m_pDictionary,n) ;
	return hr ;
}
HRESULT CHI_IniPareser::dump_ini(char* pFileNmae)
{
	HRESULT hr = 0; 
	if (m_iniState == FALSE)
	{
		return HI_ERROR_INIP_NOTINI ;//not init m_pDictionary
	}

	char *pflienameInter = pFileNmae ;
	if (pflienameInter == NULL)
	{
		pflienameInter = m_pFileName ;
	}

	if (m_pfile != NULL)
	{
		fclose(m_pfile);
	}
	if (_access(pflienameInter,2) != 0)
	{
		return HI_ERROR_INIP_READEONLYFILE ;
	}

	m_pfile = fopen(pflienameInter,"w");
	iniparser_dump_ini(m_pDictionary ,m_pfile) ;
	fclose(m_pfile);
	m_pfile = NULL ;
	return hr ;
}

/*
HRESULT CHI_IniPareser::dump(char* pFileNmae)
{
	HRESULT hr = 0; 
	return hr ;
}
*/

HRESULT CHI_IniPareser::getstr(char * key,char** ppStrReturn)
{
	HRESULT hr = 0;
	if (key == NULL||ppStrReturn == NULL)
	{
		return E_INVALIDARG ;//not init m_pDictionary
	}
	if (m_iniState == FALSE)
	{
		return HI_ERROR_INIP_NOTINI ;//not init m_pDictionary
	}
	*ppStrReturn = iniparser_getstr(m_pDictionary ,key);

	return hr ;
}

HRESULT CHI_IniPareser::getstring( char * key,char** ppStrReturn)
{
	HRESULT hr = 0;
	if (key == NULL||ppStrReturn == NULL)
	{
		return E_INVALIDARG ;//not init m_pDictionary
	}
	if (m_iniState == FALSE)
	{
		return HI_ERROR_INIP_NOTINI ;//not init m_pDictionary
	}
	*ppStrReturn = iniparser_getstring(m_pDictionary ,key ,"Not Found");

	return hr ;
}

HRESULT CHI_IniPareser::getint(char * key,int& retValue)
{
	HRESULT hr = 0;
	if (key == NULL)
	{
		return E_INVALIDARG ;//not init m_pDictionary
	}
	if (m_iniState == FALSE)
	{
		return HI_ERROR_INIP_NOTINI ;//not init m_pDictionary
	}

	retValue = iniparser_getint(m_pDictionary, key, -1) ;
	return hr ;
}

HRESULT CHI_IniPareser::getdouble( char * key, double& retValue)
{
	HRESULT hr = 0; 
	if (key == NULL)
	{
		return E_INVALIDARG ;//not init m_pDictionary
	}
	if (m_iniState == FALSE)
	{
		return HI_ERROR_INIP_NOTINI ;//not init m_pDictionary
	}

	retValue = iniparser_getdouble(m_pDictionary, key, -0.000001) ;
	return hr ;
}

HRESULT CHI_IniPareser::getboolean(char * key, BOOL& returnValue)
{
	HRESULT hr = 0;
	if (key == NULL)
	{
		return E_INVALIDARG ;
	}
	if (m_iniState == FALSE)
	{
		return HI_ERROR_INIP_NOTINI ;//not init m_pDictionary
	}

	returnValue = iniparser_getboolean(m_pDictionary, key, -1) ;
	return hr ;
}

HRESULT CHI_IniPareser::setstr(char * entry, char * val)
{
	HRESULT hr = 0;
	if (entry == NULL )
	{
		return E_INVALIDARG ;
	}
	if (m_iniState == FALSE)
	{
		return HI_ERROR_INIP_NOTINI ;//not init m_pDictionary
	}
	hr = iniparser_setstr(m_pDictionary, entry , val) ;
	return hr ;
}

HRESULT CHI_IniPareser::unset(char * entry)
{
	HRESULT hr = 0;
	if (entry == NULL)
	{
		return E_INVALIDARG ;
	}
	if (m_iniState == FALSE)
	{
		return HI_ERROR_INIP_NOTINI ;//not init m_pDictionary
	}
	iniparser_unset(m_pDictionary,entry);
	return hr ;
}

HRESULT CHI_IniPareser::find_entry(char * entry)
{
	HRESULT hr = -1;
	if (entry == NULL)
	{
		return E_INVALIDARG ;
	}
	if (m_iniState == FALSE)
	{
		return HI_ERROR_INIP_NOTINI ;//not init m_pDictionary
	}
	int ret = 
	iniparser_find_entry(m_pDictionary ,entry) ;
	if (ret == 1)
	{
		hr = 0 ;
	}
	else
	{
		hr = -1 ;
	}
	return hr ;
}

HRESULT CHI_IniPareser::loadFile(char * pininame)
{
	HRESULT hr = 0; 
	if (pininame == NULL)
	{
		return E_INVALIDARG ;
	}
	if (m_iniState == TRUE)
	{
		return HI_ERROR_INIP_HASBEENINI ;//not init m_pDictionary
	}

	m_pfile = fopen(pininame,"r");
	if (m_pfile == NULL)
	{
//		AfxMessageBox("Can not open file");
		return E_INVALIDARG ;//无效的文件名
	}
	fclose(m_pfile) ;
	m_pfile = NULL ;
	m_pFileName = (char*)malloc(strlen(pininame)+1) ;
	strcpy(m_pFileName ,pininame) ;
	m_pDictionary =  iniparser_load(pininame);
	m_iniState  = TRUE ;

	return hr ;
}

HRESULT CHI_IniPareser::freedict()
{
	HRESULT hr = 0;
	
	if (m_pDictionary != NULL)
	{
		iniparser_freedict(m_pDictionary);
		m_iniState = FALSE ;
		m_pDictionary = NULL ;
		return hr ;
	}
	else
	{
		return HI_ERROR_INIP_NOTINI; //已Free
	};
}