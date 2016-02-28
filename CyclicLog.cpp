//////////////////////////////////////////////////////////////////////
// CyclicLog.cpp: implementation of the CCyclicLog class.
//////////////////////////////////////////////////////////////////////
// Created by Ilan Gavriel 
//////////////////////////////////////////////////////////////////////

// include the following line if compiling an application with precompiled headers
#include "stdafx.h"

///////////////////////////////////////////////////////////////////////////////
#ifdef _MFC_VER
	#pragma message("CCyclicLog - Compiling for MFC")
#else
	#include <windows.h>
	#include <stdio.h>
	#include <crtdbg.h>
	#include <tchar.h>
	#pragma message("CCyclicLog - Compiling for Win32")
#endif
///////////////////////////////////////////////////////////////////////////////

#include "CyclicLog.h"
#include <iostream>
#include <exception>

using namespace std;
using namespace Logger;

//========================================================================
// MACRO - predefined macro for local usage
//========================================================================
#define LOG4J_END_LINE	_T("</log4j:event>")
#define XML_END_LINE	_T("</log>")
#define XML_EOF			_T("</logs>")

#define XSL_FILENAME	_T("CyclicLog.xsl")

#define XML_HEADER		_T("<?xml version='1.0'?>\n")													\
						_T("<?xml-stylesheet type='text/xsl' href='") XSL_FILENAME _T("'?>\n")			\
						_T("<logs>\n") XML_EOF

#define HTML_SEPARATOR	_T("</td><td>")
#define HTML_END_LINE	_T("</td></tr>")
#define HTML_EOF		_T("</table>")

#define HTML_HEADER		_T("<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.01 Transitional//EN'>\n")			\
		_T("<html><head><meta http-equiv='Content-Type' content='text/html;charset=iso-8859-1'>\n")		\
		_T("<table class=MsoTableGrid border=0 cellspacing=1 cellpadding=0>\n")							\
			_T("<thead style='background-color:steelblue;color:white'><b>")								\
			_T("<th>PID</th>")																			\
			_T("<th>TID</th>")																			\
			_T("<th>Date Time</th>")																	\
			_T("<th>Severity</th>")																		\
			_T("<th>Category</th>")																		\
			_T("<th>Event Log</th>")																	\
			_T("<th>File Name</th>")																	\
			_T("</b></thead>\n")																		\
			_T("<tfoot style='background-color:steelblue;color:white'><b>")								\
			_T("<th>PID</th>")																			\
			_T("<th>TID</th>")																			\
			_T("<th>Date Time</th>")																	\
			_T("<th>Severity</th>")																		\
			_T("<th>Category</th>")																		\
			_T("<th>Event Log</th>")																	\
			_T("<th>File Name</th>")																	\
			_T("</b></tfoot >\n")																		\
		HTML_EOF

#define XSL_FILE	_T("<?xml version='1.0' encoding='UTF-8'?>\n")										\
		_T("<xsl:stylesheet version='1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>\n")			\
		_T("<xsl:template match='/'>\n")																\
		_T("<html>\n")																					\
		_T("<head><h3>CyclicLog Log Messages</h3></head>\n")											\
		_T("<body bgcolor='white' topmargin='6' leftmargin='6'>\n")										\
			_T("<table cellspacing='0' cellpadding='0' border='1' bordercolor='#336699' ")				\
					_T("width='100%%' font-family='arial,sans-serif' font-size='x-small'>\n")			\
				_T("<tr bgcolor='#336699' text-align='left'>\n")										\
					_T("<th>Time</th>\n")																\
					_T("<th>Process</th>\n")															\
					_T("<th>Thread</th>\n")																\
					_T("<th>Severity</th>\n")															\
					_T("<th>Module</th>\n")																\
					_T("<th>File:Line</th>\n")															\
					_T("<th>Message</th>\n")															\
				_T("</tr>\n")																			\
				_T("<xsl:for-each select='logs/log'>\n")												\
					_T("<tr>\n")																		\
						_T("<td><xsl:value-of select='Date'/></td>\n")									\
						_T("<td><xsl:value-of select='@pid'/></td>\n")									\
						_T("<td><xsl:value-of select='@tid'/></td>\n")									\
						_T("<td><xsl:value-of select='Severity'/></td>\n")								\
						_T("<td><xsl:value-of select='Module'/></td>\n")								\
						_T("<td><xsl:value-of select='File'/></td>\n")									\
						_T("<td><xsl:value-of select='Msg'/></td>\n")									\
					_T("</tr>\n")																		\
				_T("</xsl:for-each>\n")																	\
			_T("</table>\n")																			\
		_T("</body>\n")																					\
		_T("</html>\n")																					\
		_T("</xsl:template>\n")																			\
		_T("</xsl:stylesheet>\n")

#define _TRACE_MSG(_msg_, _error_)																		\
	 {																									\
		TCHAR szMsgBuffer[MAX_MSG_LENGTH] = { _T('\0') };												\
		if(NO_ERROR == _error_)																			\
		{																								\
			_sntprintf_s(szMsgBuffer, MAX_MSG_LENGTH, _T("CyclicLog: %s. File: %s(%d)\n"),				\
						_msg_, __TFILE__, __LINE__);													\
		}																								\
		else																							\
		{																								\
			_sntprintf_s(szMsgBuffer, MAX_MSG_LENGTH, _T("ERROR(%d): CyclicLog: %s. File: %s(%d)\n"),	\
						_error_, _msg_, __TFILE__, __LINE__);											\
		}																								\
		szMsgBuffer[MAX_MSG_LENGTH-1] = _T('\0');														\
		OutputDebugString(szMsgBuffer);																	\
	}

#define TRACE_MSG(_msg_)			_TRACE_MSG(_msg_, NO_ERROR)
#define TRACE_ERROR(_msg_, _error_)	_TRACE_MSG(_msg_, _error_)

#define SAFE_CLOSE_HANDLE(_handle_)																		\
	if (_handle_)																						\
	{																									\
		if ( !CloseHandle(_handle_) )																	\
		{																								\
			TRACE_ERROR( _T("Failed to close handle"), GetLastError() );								\
		}																								\
		_handle_ = NULL;																				\
	}

#define CHECK_FILE_RETURN_FALSE(_file_, _msg_)															\
	if( !_file_)																						\
	{	/* make sure the log file exists */																\
		tstring strError = _T("ERROR: CyclicLog: File doesn't exist: ");								\
		strError += _msg_;																				\
		TRACE_MSG( strError.c_str() );																	\
		return false;																					\
	}

#define WRITE_INI_HEADER(_of_, _section_)		_of_ << std::endl << _T("[") << _section_ << _T("]") << std::endl;
#define WRITE_INI_PARAM(_of_, _key_, _value_)	_of_ << _key_ << _T("=") << _value_ << std::endl;

//========================================================================
// Initialization of static members
//========================================================================
StringVector	CCyclicLog::s_xSeverities;
StringVector	CCyclicLog::s_xSeveritiesXml;
StringVector	CCyclicLog::s_xSeveritiesXmlPrefix;
StringVector	CCyclicLog::s_xSeveritiesHtmlDarkPrefix;
StringVector	CCyclicLog::s_xSeveritiesHtmlLightPrefix;
StringVector	CCyclicLog::s_xSeveritiesLog4jPrefix;

const TCHAR		CLogConfigFile::s_szConfigSettings[] =	_T("Settings");
const TCHAR		CLogConfigFile::s_szConfigCategory[] =	_T("Category");

const TCHAR		CLogConfigFile::s_szConfigAppendOldFile[] =		_T("AppendOldFile");
const TCHAR		CLogConfigFile::s_szConfigBinaryMode[] =		_T("BinaryMode");
const TCHAR		CLogConfigFile::s_szConfigReportStdOut[] =		_T("ReportStdOut");
const TCHAR		CLogConfigFile::s_szConfigOutputDebugString[] =	_T("OutputDebugString");
const TCHAR		CLogConfigFile::s_szConfigFileSize_MB[] =		_T("FileSize_MB");
const TCHAR		CLogConfigFile::s_szConfigNumOfFiles[] =		_T("NumOfFiles");
const TCHAR		CLogConfigFile::s_szConfigDelimiter[] =			_T("Delimiter");
const TCHAR		CLogConfigFile::s_szConfigLogType[] =			_T("LogType");
const TCHAR		CLogConfigFile::s_szConfigExtension[] =			_T("Extension");
const TCHAR		CLogConfigFile::s_szConfigSeverityLevel[] =		_T("SeverityLevel");
const TCHAR		CLogConfigFile::s_szConfigCurrentFileNum[] =	_T("CurrentFileNumber");

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//========================================================================

/* @name CCyclicLog::CCyclicLog - Constructor
** @param const TCHAR* szLogFileName - Log FileName (without Extension)
** @param const TCHAR* szLogFilePath - Log File Path */
CCyclicLog::CCyclicLog(const TCHAR* szLogFileName, const TCHAR* szLogFilePath) /* throw(std::exception) */
 : m_bActive(true),
	m_bCycle(true),
	m_bChangeSettings(false),
	m_bNewSeverity(false),
	m_pLogFile(NULL),
	m_bMutexAlreadyExists(FALSE),
	m_dwLogThreadId(0),
	m_dwConfigThreadId(0),
	m_hLogThread(NULL),
	m_hConfigThread(NULL),
	m_hLogMutex(NULL),
	m_hNewLogEvent(NULL),
	m_hQuitEvent(NULL),
	m_pCurrentSeverities(NULL),
	m_pCurrentSeveritiesPrefix(NULL)
{
	m_xLogData.strLogFileName = szLogFileName ? szLogFileName : _T("");
	m_xLogData.strLogFilePath = szLogFilePath ? szLogFilePath : _T("");
	if(!InitializeMutex())
	{
		throw std::exception( "Failed to Initialize Mutex object" );
	}
	if(!Init())
	{
		throw std::exception( "Failed to initialize CyclicLog" );
	}
}

//========================================================================

/* @name CCyclicLog::CCyclicLog - Constructor
** @param const SLogData &xData - struct to init the CCyclicLog */
CCyclicLog::CCyclicLog(const SLogData &xLogData) /* throw(std::exception) */
 : m_bActive(true),
	m_bCycle(true),
	m_bChangeSettings(false),
	m_bNewSeverity(false),
	m_xLogData(xLogData),
	m_pLogFile(NULL),
	m_bMutexAlreadyExists(FALSE),
	m_dwLogThreadId(0),
	m_dwConfigThreadId(0),
	m_hLogThread(NULL),
	m_hConfigThread(NULL),
	m_hLogMutex(NULL),
	m_hNewLogEvent(NULL),
	m_hQuitEvent(NULL),
	m_pCurrentSeverities(NULL),
	m_pCurrentSeveritiesPrefix(NULL)
{	
	if(!InitializeMutex())
	{
		throw std::exception( "Failed to Initialize Mutex object" );
	}
	if(!Init())
	{
		throw std::exception( "Failed to initialize CyclicLog" );
	}
}

//========================================================================

/* @name CCyclicLog::~CCyclicLog - Destructor */
CCyclicLog::~CCyclicLog()
{
	TRACE_MSG( _T("Destroy CCyclicLog") );
	// deactivate the Log Object
	ActivateLog(false);
	// Set Quit Event
	if ( m_hQuitEvent )
	{
		SetEvent(m_hQuitEvent);
	}
	// Wait until the thread will done then kill it
	if (m_hLogThread)
	{
		TRACE_MSG( _T("Waiting for Log Thread...") );
		DWORD dwResult = WaitForSingleObject(m_hLogThread, /*INFINITE*/ DESTRUCT_TIMEOUT);
		if ( WAIT_TIMEOUT == dwResult )
		{
			TRACE_MSG( _T("Brute thread to close") );
			TerminateThread(m_hLogThread, 1);
		}
		m_hLogThread = NULL; //no need to delete it
	}

	if (m_hConfigThread)
	{
		TRACE_MSG( _T("Waiting for Config Thread...") );
		DWORD dwResult = WaitForSingleObject(m_hConfigThread, /*INFINITE*/ DESTRUCT_TIMEOUT);
		if ( WAIT_TIMEOUT == dwResult )
		{
			TRACE_MSG( _T("Brute thread to close") );
			TerminateThread(m_hConfigThread, 1);
		}
		m_hConfigThread = NULL; //no need to delete it
	}

	// Take control of the mutex before doing anything
	CMutexLock xLock(m_hLogMutex);
	bool bRetVal = xLock.Lock(MAX_LOCK_TIMEOUT);
	if(bRetVal)
	{
//[DEBUG]// force change settings to write a new config file
//[DEBUG]// m_bChangeSettings = true;
		WriteConfigFile();

		// save and close the log file
		if (m_pLogFile)
		{
			fflush(m_pLogFile);
			fclose(m_pLogFile);
			m_pLogFile = NULL;
		}
		bRetVal = xLock.Unlock();
	}
	if(!bRetVal)
	{
		TRACE_MSG( _T("CCyclicLog - Skip updating log config file (Mutex Lock)") );
	}

	// close the opened mutex handle
	if (m_hLogMutex)
	{
		TRACE_MSG( _T("Waiting for Mutex...") );
		DWORD dwResult = WaitForSingleObject(m_hLogMutex, /*INFINITE*/ DESTRUCT_TIMEOUT);
		if ( WAIT_TIMEOUT == dwResult )
		{
			TRACE_MSG( _T("Brute Mutex close") );
		}
		SAFE_CLOSE_HANDLE(m_hLogMutex);
	}

	// Close all Event handles
	SAFE_CLOSE_HANDLE(m_hQuitEvent);
	SAFE_CLOSE_HANDLE(m_hNewLogEvent);

}

//========================================================================

/* @name CCyclicLog::Init - An Init method 
** @return bool  */
bool CCyclicLog::Init() /* throw(std::exception) */
{
	// set the log file path
	if( m_xLogData.strLogFilePath.length() )
	{
		m_strFileName = m_xLogData.strLogFilePath;
	}
	else
	{
		TCHAR szCurrentPath[MAX_PATH] = { _T('\0') };
		::GetCurrentDirectory(MAX_PATH, szCurrentPath);
		m_strFileName = szCurrentPath;
	}
	// add a backslash char to the end of path only if needed
	if( m_strFileName[ m_strFileName.length()-1 ] != _T('\\') )
	{
		m_strFileName += _T('\\');
	}
	// Set the log file name or set the default
	m_strFileName += m_xLogData.strLogFileName.length() ? m_xLogData.strLogFileName : _T("LogFile");

	// set the configuration file name then read it (if exists)
	m_xLogConfigFile.Init( m_strFileName + _T(".ini") );
	if(! m_xLogConfigFile.ReadConfigFile(m_xLogData, m_xCategoryMap) )
	{	// Failed to read to config file - force change settings to write a new config file
		m_bChangeSettings = true;
		WriteConfigFile();
	}
	// Get extension from the log data - for backward compatible
	if( m_xLogData.strLogExtension.empty() )
	{	// Set extension according to log type if extension is empty
		switch(m_xLogData.eLogType)
		{
		case LogTypeText:
		case LogTypeTextNoCategory:
			m_xLogData.strLogExtension = _T(".log");
			break;
		case LogTypeMsgList:
		case LogTypeMsgRaw:
			m_xLogData.strLogExtension = _T(".txt");
			break;
		case LogTypeXml:
		case LogTypeLog4j:
			m_xLogData.strLogExtension = _T(".xml");
			break;
		case LogTypeHtmlLight:
		case LogTypeHtmlDark:
			m_xLogData.strLogExtension = _T(".html");
			break;
		}
	}
	m_strExtension = m_xLogData.strLogExtension;

	// Set absolute file name
	m_strAbsoluteFileName = m_strFileName + m_strExtension;

	// Set the delimiter
	m_strDelimiter = m_xLogData.strDelimiter.length() ? m_xLogData.strDelimiter : _T("\t");

	// If file size is unlimited set the cycle flag to no cycle
	if ( 0==m_xLogData.lFileSize)
	{
		m_bCycle = false;
	}
	// set the last file number the application used 
	if(m_xLogData.nCurrentFileNumber >= m_xLogData.nNumOfFiles)
	{
		m_xLogData.nCurrentFileNumber = 1;
	}
	if(m_xLogData.bReportStdOut)
	{
		SetConsoleTitle(m_xLogData.strLogFileName.length() ?
						m_xLogData.strLogFileName.c_str() : _T("LogFile") );
	}

	// Prepare the file open status
	static const size_t STATUS_REAL_LENGTH = sizeof(m_cFileStatus)*sizeof(TCHAR);
	memset(m_cFileStatus, 0, STATUS_REAL_LENGTH);
	m_cFileStatus[0] = _T('w');

	// point to the relevant severity vector
	m_pCurrentSeverities = &CCyclicLog::s_xSeverities;          
	switch(m_xLogData.eLogType)
	{
	case LogTypeXml:
		m_pCurrentSeverities = &CCyclicLog::s_xSeveritiesXml;          
		m_pCurrentSeveritiesPrefix = &CCyclicLog::s_xSeveritiesXmlPrefix;
		break;
	case LogTypeHtmlDark:
		m_pCurrentSeveritiesPrefix = &CCyclicLog::s_xSeveritiesHtmlDarkPrefix;
		break;
	case LogTypeHtmlLight:
		m_pCurrentSeveritiesPrefix = &CCyclicLog::s_xSeveritiesHtmlLightPrefix;
		break;
	case LogTypeLog4j:
		m_pCurrentSeveritiesPrefix = &CCyclicLog::s_xSeveritiesLog4jPrefix;
		break;
	}

	// append old file according to its log type
	if (m_xLogData.bAppendOldFile)
	{
		FILE* pFile = NULL;
		switch(m_xLogData.eLogType)
		{
		// If we want to append to the previous file we rename the file
		// name to save the data we can't append to an old XML/HTML file
		case LogTypeXml:
		case LogTypeHtmlDark:
		case LogTypeHtmlLight:
		case LogTypeLog4j:
		 	// We check that the file exist
			pFile = _tfopen(m_strAbsoluteFileName.c_str(), _T("r") );
			if(pFile)
			{	// file exists, close it and create a new file
				fclose(pFile);
				RenameFileName();
			}			
			break;

		default:	// all other log types
			m_cFileStatus[0] = _T('a');
			break;
		}
	}
	// set binary mode in case its active
	if (m_xLogData.bBinaryMode)
	{
		m_cFileStatus[1] = _T('b');
	}

	// finally resume the log thread
	if(m_hLogThread)
	{
		ResumeThread(m_hLogThread);
	}
	if(m_hConfigThread)
	{
		ResumeThread(m_hConfigThread);
	}
	return true;
}

//========================================================================

/* @name CCyclicLog::InitializeMutex -
** @return bool  - */
bool CCyclicLog::InitializeMutex() /* throw(std::exception) */
{
	CCyclicLog::_InitializeSeverities();

	if( !m_hQuitEvent )
	{	// Create the quit event only once
		m_hQuitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);	// Manual Reset
	}
	if ( !m_hNewLogEvent )
	{	// Create the mew log event only once
		m_hNewLogEvent = CreateEvent(NULL, FALSE, FALSE, NULL);	// Auto Reset
	}
	if( !m_hLogMutex )
	{	// Creates a Kernel Object (Mutex) as the Critical Section Object only once
		m_strMutexName = tstring(_T("Log_")) + m_xLogData.strLogFileName;
		m_hLogMutex = CreateMutex(NULL, FALSE, m_strMutexName.c_str());
		if( !m_hLogMutex )
		{
			TRACE_ERROR(_T("Failed to CreateMutex"), GetLastError());
			throw std::exception("Failed to CreateMutex");
		}
		m_bMutexAlreadyExists = ( ERROR_ALREADY_EXISTS==GetLastError() );
	}

	// Begin a new Thread suspended
	m_hLogThread = CreateThread(NULL, NULL, CCyclicLog::_LogThread, this, CREATE_SUSPENDED, &m_dwLogThreadId);
	if( !m_hLogThread )
	{
		TRACE_ERROR(_T("Failed to CreateThread _LogThread"), GetLastError());
		throw std::exception("Failed to CreateThread _LogThread");
	}
	m_hConfigThread = CreateThread(NULL, NULL, CCyclicLog::_ConfigThread, this, CREATE_SUSPENDED, &m_dwConfigThreadId);
	if( !m_hConfigThread )
	{
		TRACE_ERROR(_T("Failed to CreateThread _ConfigThread"), GetLastError());
		throw std::exception("Failed to CreateThread _ConfigThread");
	}
	return true;
}

//========================================================================

/* @name CCyclicLog::_InitializeSeverities - Initialize all static severities map */
void CCyclicLog::_InitializeSeverities()
{
	static long _lSeveritiesInitialized = FALSE;
	if ( _lSeveritiesInitialized )
	{	// severities already initialized once
		return;
	}
	// set the Severities Initialized to true
	LONG lOldValue = ::InterlockedExchange(&_lSeveritiesInitialized, TRUE);

	// make sure your the one who changed the SeveritiesInitialized
	if(FALSE != lOldValue)
	{	// severities already initialized by someone else
		return;
	}

	CCyclicLog::s_xSeverities.resize(Severity_LAST);
	CCyclicLog::s_xSeverities[SeverityDebug] =		_T("Debug");
	CCyclicLog::s_xSeverities[SeverityVerbose] =	_T("Verbose");
	CCyclicLog::s_xSeverities[SeverityInfo] =		_T("Info");
	CCyclicLog::s_xSeverities[SeverityWarning] =	_T("Warning");		
	CCyclicLog::s_xSeverities[SeverityError] =		_T("Error");

	CCyclicLog::s_xSeveritiesXml.resize(Severity_LAST);
	CCyclicLog::s_xSeveritiesXml[SeverityDebug]	=	_T("<Severity>Debug</Severity>");
	CCyclicLog::s_xSeveritiesXml[SeverityVerbose] = _T("<Severity>Verbose</Severity>");
	CCyclicLog::s_xSeveritiesXml[SeverityInfo] =	_T("<Severity>Info</Severity>");
	CCyclicLog::s_xSeveritiesXml[SeverityWarning] = _T("<Severity>Warning</Severity>");
	CCyclicLog::s_xSeveritiesXml[SeverityError]	=	_T("<Severity>Error</Severity>");

	CCyclicLog::s_xSeveritiesXmlPrefix.resize(Severity_LAST);
	CCyclicLog::s_xSeveritiesXmlPrefix[SeverityDebug] =		_T("<log level=\"1\" ");
	CCyclicLog::s_xSeveritiesXmlPrefix[SeverityVerbose] =	_T("<log level=\"2\" ");
	CCyclicLog::s_xSeveritiesXmlPrefix[SeverityInfo] =		_T("<log level=\"3\" ");
	CCyclicLog::s_xSeveritiesXmlPrefix[SeverityWarning] =	_T("<log level=\"4\" ");
	CCyclicLog::s_xSeveritiesXmlPrefix[SeverityError] =		_T("<log level=\"5\" ");

	CCyclicLog::s_xSeveritiesHtmlLightPrefix.resize(Severity_LAST);
	CCyclicLog::s_xSeveritiesHtmlLightPrefix[SeverityDebug] =	_T("<tr style='background:white;color:teal'><td>");
	CCyclicLog::s_xSeveritiesHtmlLightPrefix[SeverityVerbose] =	_T("<tr style='background:white;color:black'><td>");
	CCyclicLog::s_xSeveritiesHtmlLightPrefix[SeverityInfo] =	_T("<tr style='background:white;color:green'><td>");
	CCyclicLog::s_xSeveritiesHtmlLightPrefix[SeverityWarning] =	_T("<tr style='background:white;color:orange'><td>");
	CCyclicLog::s_xSeveritiesHtmlLightPrefix[SeverityError]	=	_T("<tr style='background:white;color:red'><td>");

	CCyclicLog::s_xSeveritiesHtmlDarkPrefix.resize(Severity_LAST);
	CCyclicLog::s_xSeveritiesHtmlDarkPrefix[SeverityDebug] =	_T("<tr style='background:black;color:white'><td>");
	CCyclicLog::s_xSeveritiesHtmlDarkPrefix[SeverityVerbose] =	_T("<tr style='background:black;color:aqua'><td>");
	CCyclicLog::s_xSeveritiesHtmlDarkPrefix[SeverityInfo] =		_T("<tr style='background:black;color:lime'><td>");
	CCyclicLog::s_xSeveritiesHtmlDarkPrefix[SeverityWarning] =	_T("<tr style='background:black;color:orange'><td>");
	CCyclicLog::s_xSeveritiesHtmlDarkPrefix[SeverityError] =	_T("<tr style='background:black;color:red'><td>");

	CCyclicLog::s_xSeveritiesLog4jPrefix.resize(Severity_LAST);
	CCyclicLog::s_xSeveritiesLog4jPrefix[SeverityDebug] =	_T("level=\"DEBUG\" ");
	CCyclicLog::s_xSeveritiesLog4jPrefix[SeverityVerbose] =	_T("level=\"DEBUG\" ");
	CCyclicLog::s_xSeveritiesLog4jPrefix[SeverityInfo] =	_T("level=\"INFO\" ");
	CCyclicLog::s_xSeveritiesLog4jPrefix[SeverityWarning] =	_T("level=\"WARN\" ");
	CCyclicLog::s_xSeveritiesLog4jPrefix[SeverityError] =	_T("level=\"ERROR\" ");
}

//========================================================================

/* @name CCyclicLog::_LogThread - This function will run in a separated thread
**		the function RunLogThread() from the given class pointer: pObject
** @param LPVOID pObject -
** @return DWORD  - */
DWORD CCyclicLog::_LogThread(LPVOID pObject)
{	// Cast the pObject parameter to CCyclicLog class
    CCyclicLog* pCyclicLog = reinterpret_cast<CCyclicLog*>(pObject);
    if (pCyclicLog == 0)
	{	// if pObject is not valid
		return 1;
	}
	pCyclicLog->RunLogThread();
	return 0;   // thread completed successfully
} 

//========================================================================

/* @name CCyclicLog::RunLogThread - This Function will be used as the Thread
**									function in endless loop with quit condition.
** @return void  */
void CCyclicLog::RunLogThread()
{
	// Take control of the mutex before doing anything
	CMutexLock xLock(m_hLogMutex);
	bool bRetVal = xLock.Lock(MAX_LOCK_TIMEOUT);
	if(bRetVal)
	{	// First Open the Log File
		// Set the File status: a=Append, w=Write
		if(m_bMutexAlreadyExists)
		{
			m_cFileStatus[0] = _T('a');
		}

		// Add the proper header
		switch(m_xLogData.eLogType)
		{
		case LogTypeXml:
			{
				m_pLogFile = _tfopen(m_strAbsoluteFileName.c_str(), _T("w+") );
				WriteHeaderToFile(XML_HEADER);
				// on the very first time create the XSL only if not exsits
				tstring strXslFile = m_xLogData.strLogFilePath;
				// add a backslash char to the end of path only if needed
				if( strXslFile.length()>0 && strXslFile[ strXslFile.length()-1 ] != _T('\\') )
				{
					strXslFile += _T('\\');
				}
				strXslFile += XSL_FILENAME;
				FILE* pXslFile = _tfopen(strXslFile.c_str(), _T("r") );
				if(!pXslFile)
				{	// file not exists - create it
					pXslFile = _tfopen(strXslFile.c_str(), _T("w+") );
					_ftprintf(pXslFile, XSL_FILE);
					fflush(pXslFile);
				}
				fclose(pXslFile);
			}
			break;
		case LogTypeHtmlDark:
		case LogTypeHtmlLight:
			m_pLogFile = _tfopen(m_strAbsoluteFileName.c_str(), _T("w+") );
			WriteHeaderToFile(HTML_HEADER);
			break;
		default:
			m_pLogFile = _tfopen(m_strAbsoluteFileName.c_str(), m_cFileStatus);
			break;
		}
		m_cFileStatus[0] = _T('w');
		// unlock the critical section - it will be lock later on demand
		bRetVal = xLock.Unlock();
	}
	if(!bRetVal)
	{
		TRACE_MSG( _T("RunLogThread() - Break the main thread - logger is disabled (Mutex Lock)") );
		return;
	}

	const unsigned int MAX_EVENTS = 2;
	HANDLE hEventsArray[MAX_EVENTS];
	hEventsArray[0] = m_hQuitEvent;
	hEventsArray[1] = m_hNewLogEvent;

	// Endless Loop while bRetVal is true for Log Events
	bool bQuit = false;
	while(!bQuit)
	{
		DWORD dwResult = WaitForMultipleObjects(MAX_EVENTS, hEventsArray, FALSE, INFINITE);
		switch(dwResult)
		{
		case WAIT_OBJECT_0:	// When receiving a Quit Event
			bQuit = true;
			///////////////////////////////////////////////////////////////
			// There is NO break command here on purpose, to allow writing
			// the rest of the Messages Queue into the Log File.
			///////////////////////////////////////////////////////////////

		case WAIT_OBJECT_0+1:	// When receiving a new log message Event
			// Take control of the mutex before doing anything
			if( xLock.Lock(ONE_SEC) )
			{	// try to empty the message queue
				while (! m_xLogMessagesQueue.empty() )
				{
					const SLogMsg& rLogMsg = m_xLogMessagesQueue.front();
					WriteMsgToFile(rLogMsg);
					m_xLogMessagesQueue.pop();
				}
				// unlock the critical section
				xLock.Unlock();
			}
			break;
		}
	}
}

//========================================================================

/* @name CCyclicLog::_ConfigThread - This function will run in a separated thread
**		the function RunConfigThread() from the given class pointer: pObject
** @param LPVOID pObject -
** @return DWORD  - */
DWORD CCyclicLog::_ConfigThread(LPVOID pObject)
{	// Cast the pObject parameter to CCyclicLog class
    CCyclicLog* pCyclicLog = reinterpret_cast<CCyclicLog*>(pObject);
    if (pCyclicLog == 0)
	{	// if pObject is not valid
		return 1;
	}
	pCyclicLog->RunConfigThread();
	return 0;   // thread completed successfully
} 

//========================================================================

/* @name CCyclicLog::RunConfigThread - This Function will be used as the Thread
**									function in endless loop with quit condition.
** @return void  */
void CCyclicLog::RunConfigThread()
{
	// Endless Loop while bRetVal is true for Log Events
	bool bQuit = false;
	while(!bQuit)
	{
		DWORD dwResult = WaitForSingleObject(m_hQuitEvent, ONE_SEC);
		switch(dwResult)
		{
		case WAIT_OBJECT_0:	// When receiving a Quit Event
			bQuit = true;
			break;

		case WAIT_TIMEOUT:	// When timeout
		default:
			// read INI file again because file time changed
			ReadConfigFile(ONE_SEC);
			// after reading, write new categories
			// if the setteing were changed or new severity was added - then write the entire file
			if (m_bChangeSettings || m_bNewSeverity)
			{
				WriteConfigFile(ONE_SEC);
			}
			break;
		}
	}
}

//========================================================================

/* @name CCyclicLog::SetSeverityLevel - Set The Minimal Severity Level,
**										which the Log Object will accept
** @param const int nSeverity -
** @param const tstring& strCategory - */
void CCyclicLog::SetSeverityLevel(const int nSeverity, const tstring& strCategory)
{
 	// Take control of the mutex before doing anything
	CMutexLock xLock(m_hLogMutex);
	bool bRetVal = xLock.Lock(ONE_SEC);
	if(bRetVal)
	{	// now its safe to change the severity inside the map
		if(strCategory.length())
		{	// if category is not empty - set its new severity
			if(	strCategory.length() <= CATEGORY_LENGTH)
			{
				m_xCategoryMap[strCategory] = CCyclicLog::_GetSeverityValue(nSeverity);
			}
			else	// truncate the category length if needed
			{
				tstring strCategoryTrunc(strCategory);
				strCategoryTrunc.erase(CATEGORY_LENGTH);
				m_xCategoryMap[strCategoryTrunc] = CCyclicLog::_GetSeverityValue(nSeverity);
			}
		}
		else
		{	// if category is not given - set severity for all categories
			m_xLogData.eSeverityLevel = CCyclicLog::_GetSeverityValue(nSeverity);
			for(CategoryMap::iterator iter = m_xCategoryMap.begin();
				iter != m_xCategoryMap.end();
				iter++)
			{
				SeverityTypes& eSeverity = iter->second;
				eSeverity = m_xLogData.eSeverityLevel;
			}
		}
		// finally set the new severity flag to true
		m_bNewSeverity = true;
	}
}

//========================================================================

/* @name CCyclicLog::SetSeverityLevel - Set The Minimal Severity Level,
**										 which the Log Object will accept
**  @param const int nSeverityLevel -
**  @param const TCHAR* szCategory - */
void CCyclicLog::SetSeverityLevel(const int nSeverityLevel, const TCHAR* szCategory)
{
	tstring strCategory = szCategory ? szCategory : _T("");
	SetSeverityLevel(nSeverityLevel, strCategory);
}

//========================================================================

/* @name CCyclicLog::GetCategoryMap - Get a copy of the entire Category Map
** @param CategoryMap& rCategoryMap -
** @return bool  - */
bool CCyclicLog::GetCategoryMap(CategoryMap& rCategoryMap) const
{ 
 	// Take control of the mutex before doing anything
	CMutexLock xLock(m_hLogMutex);
	bool bRetVal = xLock.Lock(ONE_SEC);
	if(bRetVal)
	{	// now its safe to copy the Category Map
		rCategoryMap = m_xCategoryMap;
	}
	return bRetVal;
}

//========================================================================

/* @name CCyclicLog::IsEffective - Check if Severity Level is effective
** @param const int nSeverityLevel -
** @param const tstring& strCategory -
** @return bool  - */
bool CCyclicLog::IsEffective(const int nSeverityLevel, const tstring& strCategory)
{
	// make sure the Severity is not OFF
	if (SeverityOff==nSeverityLevel)
	{
		return false;
	}
 	// Take control of the mutex before doing anything
	CMutexLock xLock(m_hLogMutex);
	bool bRetVal = xLock.Lock(ONE_SEC);
	if(!bRetVal)
	{	// assume its not effective, cause you didn't get a chance to look inside the map
		return false;
	}
	// lock was succeeded - now check the map 

	// get a reference to the input category
	tstring& rNonCostCategory = const_cast<tstring&>(strCategory);
	// truncate the input category if needed
	tstring strCategoryTrunc;
	if(	strCategory.length() > CATEGORY_LENGTH)
	{
		strCategoryTrunc = strCategory;
		strCategoryTrunc.erase(CATEGORY_LENGTH);
		// set the reference to the new truncated category
		rNonCostCategory = strCategoryTrunc;
	}
	// find the proper category (original or truncated) in the map
	CategoryMap::const_iterator iter = m_xCategoryMap.find( rNonCostCategory );
	if( iter == m_xCategoryMap.end() )
	{	// if category not exist - add it with the current severity
		if ( rNonCostCategory.size() )
		{
			SetSeverityLevel(m_xLogData.eSeverityLevel, rNonCostCategory);
		}
	}
	else
	{	// if category exist - check if its valid
		const SeverityTypes& eSeverity = iter->second;
		return ( nSeverityLevel >= eSeverity );
	}
	return (nSeverityLevel >= m_xLogData.eSeverityLevel);
}

//========================================================================

/* @name CCyclicLog::IsEffective - Check if Severity Level is effective
** @param const SLogMsg& xLogMsg - */
bool CCyclicLog::IsEffective(const SLogMsg& xLogMsg)
{
	return IsEffective(xLogMsg.nSeverity, xLogMsg.strCategory);
}

//========================================================================

/* @name CCyclicLog::IsEffective - Check if Severity Level is effective
** @param const int nSeverityLevel -
** @param const TCHAR* szCategory -
** @return bool  - */
bool CCyclicLog::IsEffective(const int nSeverityLevel, const TCHAR* szCategory)
{
	tstring strCategory = szCategory ? szCategory : _T("");
	return IsEffective(nSeverityLevel, strCategory);
}

//========================================================================

/* @name CCyclicLog::RotateLogFile - Force file rotation NOW (with timeout option)
** @param const DWORD dwMilliseconds - timeout option
** @return bool  */
bool CCyclicLog::RotateLogFile(const DWORD dwMilliseconds)
{
	// Take control of the mutex before doing anything
	CMutexLock xLock(m_hLogMutex);
	bool bRetVal = xLock.Lock(dwMilliseconds);
	if(bRetVal)
	{
		bRetVal = RotateAction();
	}
	return bRetVal;
}

//========================================================================

/* @name CCyclicLog::RotateAction - The Rotate action itself
**  @return bool  */
bool CCyclicLog::RotateAction()
{
	// make sure the log file exists
	CHECK_FILE_RETURN_FALSE(m_pLogFile, _T("RotateAction") );

	// Take control of the mutex before doing anything
	CMutexLock xLock(m_hLogMutex);
	bool bRetVal = xLock.Lock(MAX_LOCK_TIMEOUT);
	if(bRetVal)
	{
		// first close the current file
		fflush(m_pLogFile);
		fclose(m_pLogFile);
		m_pLogFile = NULL;

		// then rename it
		bRetVal = RenameFileName();

		// open file for Write, in the file status mode
		m_pLogFile = _tfopen(m_strAbsoluteFileName.c_str(), m_cFileStatus);
		if( !m_pLogFile )
		{	// file was not created set return value to false
			TRACE_ERROR(_T("RotateAction - Failed to open file"), GetLastError());
			bRetVal = false;
		}
		else
		{ 	// Add the proper header
			switch(m_xLogData.eLogType)
			{
			case LogTypeXml:
				WriteHeaderToFile(XML_HEADER);
				break;
			case LogTypeHtmlDark:
			case LogTypeHtmlLight:
				WriteHeaderToFile(HTML_HEADER);
				break;
			}
		}
	}
	if(!bRetVal)
	{
		TRACE_MSG( _T("RotateAction() - Failed to rotate log file (Mutex Lock)") );
	}
	return bRetVal;
}

//========================================================================

/* @name CCyclicLog::LogEvent - Send a given Event into a queue of Messages
** @param const SLogMsg& xLogMsg -
** @return bool  - */
bool CCyclicLog::LogEvent(const SLogMsg& xLogMsg)
{
	// Check if the severity is in Range and the log (is activate or in
	// initialization).
	bool bRetVal = m_bActive && IsEffective(xLogMsg);
	if (!bRetVal)
	{
		return false;
	}
	static bool _sLogErrorOnce = false;
	// Take control of the mutex before doing anything
	CMutexLock xLock(m_hLogMutex);
	bRetVal = xLock.Lock(ONE_SEC);
	if(bRetVal)
	{
		try
		{	//finally put the built message in the message queue
			//Wait to the critical section before pushing into the queue
			m_xLogMessagesQueue.push(xLogMsg);
			// turn off the error bit if it was turned on
			if (_sLogErrorOnce)
			{
				_sLogErrorOnce = false;
			}
			// notify the thread about the new log message
			SetEvent(m_hNewLogEvent);
		}
		catch(const std::exception xError)
		{	// display the error message once
			if (!_sLogErrorOnce)
			{
				_sLogErrorOnce = true;	// turn on the error bit
				TRACE_ERROR(_T("Failed while insert new Messages to Queue"), GetLastError());
				TRACE_MSG( xError.what() );
				WriteMsgToFile( _T("Failed while insert new Messages to Queue") );
			}
			bRetVal = false;
		}
	}
	if(!bRetVal)
	{
		TRACE_MSG( _T("LogEvent(SLogMsg) - Failed to add event to log queue (Mutex Lock)") );
	}
	return bRetVal;
}

//========================================================================

/* @name CCyclicLog::LogEvent - Send a given Event into a queue of Messages
** @param const int	nSeverity
** @param const int	nLineNumber
** @param const TCHAR*	szFileName
** @param const TCHAR*	szCategory
** @param const TCHAR*	szLogMsgFormat
** @return bool  */
bool CCyclicLog::LogEvent(const int		nSeverity,
						  const int		nLineNumber,
						  const TCHAR*	szFileName,
						  const TCHAR*	szCategory,
						  const TCHAR*	szLogMsgFormat, ...)
{
	va_list args;
	va_start(args, szLogMsgFormat);
	bool bRetVal = LogEventV(nSeverity, nLineNumber, szFileName, szCategory,szLogMsgFormat, args);
	va_end(args);
	return bRetVal;
}

//========================================================================

/* @name CCyclicLog::LogEventV - Send a given Event into a queue of Messages
** @param const int	nSeverity
** @param const int	nLineNumber
** @param const TCHAR*	szFileName
** @param const TCHAR*	szCategory
** @param const TCHAR*	szLogMsgFormat + argument list
** @return bool  */
bool CCyclicLog::LogEventV(const int	nSeverity,
						  const int		nLineNumber,
						  const TCHAR*	szFileName,
						  const TCHAR*	szCategory,
						  const TCHAR*	szLogMsgFormat,
						  const va_list	args)
{
	TCHAR szLogMsg[MAX_MSG_LENGTH] = { _T('\0') };
	static const size_t FORMAT_LENGTH = sizeof(szLogMsg)/sizeof(TCHAR) - sizeof(TCHAR);

	int nSize = _vsntprintf(szLogMsg, FORMAT_LENGTH, szLogMsgFormat, args);
	if(nSize < 0)
	{	// force null if the number of written bytes exceeds buffer
		szLogMsg[MAX_MSG_LENGTH-1] = _T('\0');
	}
	SLogMsg xLogMsg(szLogMsg, nSeverity, szCategory, szFileName, nLineNumber);
	return LogEvent( xLogMsg );
}

//========================================================================

/* @name CCyclicLog::LogEvent - Send a given Event into a queue of Messages
** @param const TCHAR*	szLogMsg
** @param const int	nSeverity
** @param const TCHAR*	szCategory
** @param const TCHAR*	szFileName
** @param const int nLineNumber
** @return bool  */
bool CCyclicLog::LogEvent(const TCHAR*	szLogMsg,
						  const int		nSeverity,
						  const TCHAR*	szCategory,
						  const TCHAR*	szFileName,
						  const int		nLineNumber)
{
	SLogMsg xLogMsg(szLogMsg, nSeverity, szCategory, szFileName, nLineNumber);
	return LogEvent( xLogMsg );
}

//========================================================================

/* @name CCyclicLog::WriteHeaderToFile - Add an XML header to the file 
** @param const TCHAR* szMsg
** @return bool  */
bool CCyclicLog::WriteHeaderToFile(const TCHAR* szMsg)
{
	// make sure the log file exists
	CHECK_FILE_RETURN_FALSE(m_pLogFile, _T("WriteHeaderToFile") );

	// Take control of the mutex before doing anything
	CMutexLock xLock(m_hLogMutex);
	bool bRetVal = xLock.Lock(MAX_LOCK_TIMEOUT);
	if(bRetVal)
	{
		_ftprintf(m_pLogFile, _T("%s"), szMsg);
		fflush(m_pLogFile);
		if (m_bCycle)
		{
			if ( ftell(m_pLogFile) > static_cast<long>(m_xLogData.lFileSize) )
			{
				bRetVal = RotateAction();
			}
		}
	}
	if(!bRetVal)
	{
		TRACE_MSG( _T("WriteHeaderToFile(TCHAR) - Failed to write log file header (Mutex Lock)") );
	}
	return bRetVal;
}

//========================================================================

/* @name CCyclicLog::WriteMsgToFile - Write a Given message into the File
** @param const SLogMsg& xLogMsg -
** @return bool  - */
bool CCyclicLog::WriteMsgToFile(const SLogMsg& xLogMsg)
{
	CHECK_FILE_RETURN_FALSE(m_pLogFile, _T("WriteMsgToFile(SLogMsg)") );

	// Take control of the mutex before doing anything
	CMutexLock xLock(m_hLogMutex);
	if( !xLock.Lock(MAX_LOCK_TIMEOUT) )
	{
		TRACE_MSG( _T("WriteMsgToFile(SLogMsg) - Failed to writing event to log file (Mutex Lock)") );
		return false;
	}

	const SeverityTypes eSeverity = CCyclicLog::_GetSeverityValue(xLogMsg.nSeverity);

	//then build the full event message 
	TCHAR szFormatMsg[MAX_MSG_LENGTH] = { _T('\0') };
	// maximum formatted message is (MAX_MSG_LENGTH - 256) to allow header
	static const size_t FORMAT_LENGTH = MAX_MSG_LENGTH - 256;
	// build the message tstring object, and truncate it if its too long
	tstring strMsg(xLogMsg.strMessage);
	if( strMsg.length() > FORMAT_LENGTH)
	{
		strMsg.resize(FORMAT_LENGTH);
		strMsg.insert(0, _T("[Truncated] ") );
	}
	// report OutputDebugString if enabled and not empty string
	if(m_xLogData.bOutputDebugString && strMsg.length()>0)
	{
		if(strMsg[strMsg.length()-1]!='\n')
		{	// force new line for WinDbg pretty print
			tstring strDebugMsg = strMsg + tstring("\n");
			OutputDebugString( strDebugMsg.c_str() );
		}
		else
		{
			OutputDebugString( strMsg.c_str() );
		}
	}
	// format the message according to the log type
	switch(m_xLogData.eLogType)
	{
	case LogTypeXml:
		_sntprintf_s(szFormatMsg, FORMAT_LENGTH,
					_T("%s")					// Header
					_T("pid=\"%d\" ")			// process 
					_T("tid=\"%d\">")			// thread  
					_T("%s")					// severity
					_T("<Date>%s</Date>")		// Date
					_T("<Module>%s</Module>")	// Module
					_T("<Msg><![CDATA[%s]]></Msg>")	// Msg
					_T("<File>%s(%d)</File>")	// File+Line
					XML_END_LINE _T("\n")		// End Line
					XML_EOF,

					(*m_pCurrentSeveritiesPrefix)[eSeverity].c_str(), // Header
					xLogMsg.dwProcessId,						// process 
					xLogMsg.dwThreadId,							// thread  
					(*m_pCurrentSeverities)[eSeverity].c_str(),	// severity
					xLogMsg.TimeStamp().c_str(),				// Date/Time
					xLogMsg.strCategory.c_str(),				// Category
					strMsg.c_str(),								// Msg          
					xLogMsg.strFileName.c_str(),				// File
					xLogMsg.nLineNumber);						// Line
		break;

	case LogTypeHtmlDark:
	case LogTypeHtmlLight:
		_sntprintf_s(szFormatMsg, FORMAT_LENGTH,
					_T("%s")					// Header (NO Delimiter)
					_T("%d")	HTML_SEPARATOR	// process	+ Delimiter
					_T("%d")	HTML_SEPARATOR	// thread	+ Delimiter
					_T("%s")	HTML_SEPARATOR	// Date		+ Delimiter
					_T("%s")	HTML_SEPARATOR	// severity	+ Delimiter
					_T("%s")	HTML_SEPARATOR	// Module	+ Delimiter
					_T("%s")	HTML_SEPARATOR	// Msg		+ Delimiter
					_T("%s(%d)")				// File+Line
					HTML_END_LINE _T("\n")	   	// End Line
					HTML_EOF,

					(*m_pCurrentSeveritiesPrefix)[eSeverity].c_str(), // Header
					xLogMsg.dwProcessId,						// process 
					xLogMsg.dwThreadId,							// thread  
					xLogMsg.TimeStamp().c_str(),				// Date/Time
					(*m_pCurrentSeverities)[eSeverity].c_str(),	// severity
					xLogMsg.strCategory.c_str(),				// Category
					strMsg.c_str(),								// Msg          
					xLogMsg.strFileName.c_str(),				// FileName+
					xLogMsg.nLineNumber);						// LineNumber
		break;

	case LogTypeMsgList:
		_sntprintf_s(szFormatMsg, FORMAT_LENGTH, _T("%s\n"), strMsg.c_str() );
		break;

	case LogTypeMsgRaw:
		_sntprintf_s(szFormatMsg, FORMAT_LENGTH, _T("%s"), strMsg.c_str() );
		break;

	case LogTypeLog4j:
		/*
			<log4j:event logger="PID" timestamp="1329223441513" level="ERROR" thread="TID">
				<log4j:message><![CDATA[This is an Error...]]></log4j:message>
				<log4j:locationInfo class="" method="CATEGORY" file="FILE" line="LINE" />
			</log4j:event>
		*/
		// The Log4j event is larger than FORMAT_LENGTH, therefore split it into two parts
		// LOG4J Part 1
		_sntprintf_s(szFormatMsg, FORMAT_LENGTH,
					_T("<log4j:event logger=\"%d\" ")					// Header is the process 
					_T("timestamp=\"%I64d\" ")							// Date - log4j timestamp format is (time_t/1000L)
					_T("%s")											// severity
					_T("thread=\"%d\">"),								// thread  

					xLogMsg.dwProcessId,								// process
					static_cast<time_t>(xLogMsg)*1000,					// Date/Time - log4j timestamp format is (time_t/1000L)
					(*m_pCurrentSeveritiesPrefix)[eSeverity].c_str(),	// severity
					xLogMsg.dwThreadId);								// thread
		WriteMsgToFile(szFormatMsg);

		// LOG4J Part 2
		_sntprintf_s(szFormatMsg, FORMAT_LENGTH,
					_T("<log4j:message><![CDATA[%s]]></log4j:message>")	// Msg
					_T("<log4j:locationInfo class=\"\" method=\"%s\" ")	// Category
					_T("File=\"%s\" line=\"%d\" />")					// File+Line
					LOG4J_END_LINE _T("\n"),							// End Line

					strMsg.c_str(),										// Msg
					xLogMsg.strCategory.c_str(),						// Category
					xLogMsg.strFileName.c_str(),						// File
					xLogMsg.nLineNumber);								// Line
		break;
	case LogTypeTextNoCategory:
		_sntprintf_s(szFormatMsg, FORMAT_LENGTH,
					_T("%5d")	_T("%s")		// process	+ Delimiter
					_T("%5d")	_T("%s")		// thread	+ Delimiter
					_T("%s")	_T("%s")		// Date		+ Delimiter
					_T("%-7s")	_T("%s")		// severity	+ Delimiter
					_T("%s")	_T("%s")		// Msg		+ Delimiter
					_T("%s(%d)\n"),				// File+Line

					xLogMsg.dwProcessId,			m_strDelimiter.c_str(),	// process	+ Delimiter
					xLogMsg.dwThreadId,				m_strDelimiter.c_str(),	// thread	+ Delimiter
					xLogMsg.TimeStamp().c_str(),	m_strDelimiter.c_str(),	// Date/Time+ Delimiter
					(*m_pCurrentSeverities)[eSeverity].c_str(),	// severity
													m_strDelimiter.c_str(),	// severity	+ Delimiter
					strMsg.c_str(),					m_strDelimiter.c_str(),	// Msg		+ Delimiter
					xLogMsg.strFileName.c_str(),							// FileName
					xLogMsg.nLineNumber);									// LineNumber
		break;

	default:	// all other log types
		_sntprintf_s(szFormatMsg, FORMAT_LENGTH,
					_T("%5d")	_T("%s")		// process	+ Delimiter
					_T("%5d")	_T("%s")		// thread	+ Delimiter
					_T("%s")	_T("%s")		// Date		+ Delimiter
					_T("%-7s")	_T("%s")		// severity	+ Delimiter
					_T("%-15s")	_T("%s")		// Module	+ Delimiter
					_T("%s")	_T("%s")		// Msg		+ Delimiter
					_T("%s(%d)\n"),				// File+Line

					xLogMsg.dwProcessId,			m_strDelimiter.c_str(),	// process	+ Delimiter
					xLogMsg.dwThreadId,				m_strDelimiter.c_str(),	// thread	+ Delimiter
					xLogMsg.TimeStamp().c_str(),	m_strDelimiter.c_str(),	// Date/Time+ Delimiter
					(*m_pCurrentSeverities)[eSeverity].c_str(),	// severity
													m_strDelimiter.c_str(),	// severity	+ Delimiter
					xLogMsg.strCategory.c_str(),	m_strDelimiter.c_str(),	// Category + Delimiter
					strMsg.c_str(),					m_strDelimiter.c_str(),	// Msg		+ Delimiter
					xLogMsg.strFileName.c_str(),							// FileName
					xLogMsg.nLineNumber);									// LineNumber
			break;
	}
	if(m_xLogData.bReportStdOut)
	{
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
								LogColorsMap[ CCyclicLog::_GetSeverityValue(xLogMsg.nSeverity) ].wAttributes );
		tcout << szFormatMsg;
	}
	return WriteMsgToFile(szFormatMsg);
}

//========================================================================

/** @name CCyclicLog::WriteMsgToFile - Write a Given message into the File
**  @param const TCHAR* szMsg
**  @return bool  */
bool CCyclicLog::WriteMsgToFile(const TCHAR* szMsg)
{
	CHECK_FILE_RETURN_FALSE(m_pLogFile, _T("WriteMsgToFile(TCHAR)") );

	// Take control of the mutex before doing anything
	CMutexLock xLock(m_hLogMutex);
	if( !xLock.Lock(MAX_LOCK_TIMEOUT) )
	{
		TRACE_MSG( _T("WriteMsgToFile(TCHAR) - Failed to write log file header (Mutex Lock)") );
		return false;
	}
	// seek the file position according to the log type
	switch(m_xLogData.eLogType)
	{
	case LogTypeXml:
		fseek(m_pLogFile,
				-static_cast<long>( _tcsclen(XML_EOF)/sizeof(TCHAR)),
				SEEK_END);
		break;
	case LogTypeHtmlDark:
	case LogTypeHtmlLight:
		fseek(m_pLogFile,
				-static_cast<long>( _tcsclen(HTML_EOF)/sizeof(TCHAR)),
				SEEK_END);
		break;
	}
	// write the current given message
	bool bRetVal = true;
	_ftprintf(m_pLogFile, _T("%s"), szMsg);
	fflush(m_pLogFile);
	if (m_bCycle)
	{
		const unsigned long lCurrFileSize = ftell(m_pLogFile);
		if (lCurrFileSize > m_xLogData.lFileSize) 
		{
			bRetVal = RotateAction();
		}
	}
	return bRetVal;
}

//========================================================================

/* @name CCyclicLog::RenameFileName - Rename the current Log File name into a new name
**						no need to check is file exists, we just change its name
** @return bool  - */
bool CCyclicLog::RenameFileName()
{
	// Take control of the mutex before doing anything
	CMutexLock xLock(m_hLogMutex);
	int nStatus = 0;
	bool bRetVal = xLock.Lock(MAX_LOCK_TIMEOUT);
	if(bRetVal)
	{ 	// if we have limits of the number of files
		if(m_xLogData.nNumOfFiles) 
		{	// Set the next file counter
			if(++m_xLogData.nCurrentFileNumber >= m_xLogData.nNumOfFiles)
			{	// maximum file number exceeded - reset it to 1 again
				m_xLogData.nCurrentFileNumber = 1;
			}
			
			// Write the current file counter for the next time the system is up
			WriteCurrentFileNumber();

			// Convert the counter to string 
			TCHAR szBuff[MAX_INT_LENGTH] = { _T('\0') };
			_sntprintf_s(szBuff, MAX_INT_LENGTH, _T("_%03d"),m_xLogData.nCurrentFileNumber);
			tstring strNewFileName = m_strFileName + szBuff + m_strExtension;

			// Before rename the current file to its new name, try to delete
			// this file that was created in a previous cycle (if this file existing before)
			_tremove( strNewFileName.c_str() );

			// After this stage we don't add to a file 
			m_cFileStatus[0] = _T('w');

			//finally rename the log file name from the OLD name to the NEW name
			nStatus = _trename(m_strAbsoluteFileName.c_str(), strNewFileName.c_str());
			if(0!=nStatus)
			{
				TRACE_MSG( _T("RenameFileName - Failed to rename file") );
				bRetVal = false;
			}
		}
		else
		{	//	first build the time stamp
			SYSTEMTIME tTimeStamp;
			GetLocalTime(&tTimeStamp);		// Current Local time

			TCHAR szTimeBuffer[TIME_LENGTH] = { _T('\0') };
			static const size_t TIME_BUF_LEN = sizeof(szTimeBuffer)/sizeof(TCHAR) - sizeof(TCHAR);
			_sntprintf_s(szTimeBuffer, TIME_BUF_LEN, _T("%04d%02d%02d_%02d%02d%02d"),
						tTimeStamp.wYear,
						tTimeStamp.wMonth,
						tTimeStamp.wDay,
						tTimeStamp.wHour,
						tTimeStamp.wMinute,
						tTimeStamp.wSecond );

			//then build the OLD log file name
			tstring strOldFileName (m_strFileName);
			strOldFileName += m_strExtension;

			//and the NEW log file name (with the time stamp)
			tstring strNewFileName (m_strFileName);
			strNewFileName += _T("_");
			strNewFileName += szTimeBuffer;
			strNewFileName += m_strExtension;

			// Before rename the current file to its new name, try to delete
			// this file (if this file existing before)
			_tremove(strNewFileName.c_str());

			// After this stage we don't add to a file 
			m_cFileStatus[0] = _T('w');

			//finally rename the log file name from the OLD name to the NEW name
			nStatus = _trename(strOldFileName.c_str(), strNewFileName.c_str());		
			if(0!=nStatus)
			{
				TRACE_MSG( _T("RenameFileName - Failed to rename file") );
				bRetVal = false;
			}
		}
	}
	if(!bRetVal)
	{
		TRACE_MSG( _T("RenameFileName() - Failed to rename file (Mutex Lock)") );
	}
	return (bRetVal);
}

//========================================================================

/* @name CCyclicLog::WriteLastFileNumber - Write the current number of file we use
** @param const DWORD dwMilliseconds - timeout option
** @return bool  - */
bool CCyclicLog::WriteCurrentFileNumber(const DWORD dwMilliseconds)
{
  	// Take control of the mutex before doing anything
	CMutexLock xLock(m_hLogMutex);
	if( !xLock.Lock(dwMilliseconds) )
	{
		TRACE_MSG( _T("WriteLastFileNumber - Failed to get mutex") );
		return false;
	}
	bool bRetVal; 
	// if the setteing were changed or new severity was added - then write the entire file
	if (m_bChangeSettings || m_bNewSeverity)
	{
		bRetVal = WriteConfigFile(dwMilliseconds);
	}
	else
	{
		TCHAR szBuffer[MAX_INT_LENGTH] = { _T('\0') };
		_sntprintf_s(szBuffer, MAX_INT_LENGTH, _T("%d"),m_xLogData.nCurrentFileNumber);
		bRetVal = TRUE==WritePrivateProfileString(CLogConfigFile::s_szConfigSettings,
												 CLogConfigFile::s_szConfigCurrentFileNum,
												 szBuffer, m_xLogConfigFile.GetFileName() );
	}
	return bRetVal;
}

//========================================================================

/* @name CCyclicLog::ReadConfigFile - read configuration from a config file (if exists)
** @param const DWORD dwMilliseconds - timeout option
** @return bool  - */
bool CCyclicLog::ReadConfigFile(const DWORD dwMilliseconds)
{
	// Take control of the mutex before doing anything
	CMutexLock xLock(m_hLogMutex);
	if( !xLock.Lock(dwMilliseconds) )
	{
		TRACE_MSG( _T("ReadConfigFile - Failed to get mutex") );
		return false;
	}

	// get the last write time of the configuration
	if ( !ReadConfigFileTime(dwMilliseconds) )
	{	// skip file reading, cause file time didn't changed
		return true;
	}
	bool bRetVal = m_xLogConfigFile.ReadConfigFile(m_xLogData, m_xCategoryMap);
	if(bRetVal)
	{	// set the change settings flag to false
		m_bChangeSettings = false;
	}
	else
	{	// Failed to read to config file - force change settings to write a new config file
		m_bChangeSettings = true;
		bRetVal = WriteConfigFile(dwMilliseconds);
	}
	return bRetVal;
}

//========================================================================

/* @name CCyclicLog::WriteConfigFile - write the configuration to a config file
** @param const DWORD dwMilliseconds - timeout option
** @return bool  - */
bool CCyclicLog::WriteConfigFile(const DWORD dwMilliseconds)
{
	// write config file only if needed
	if (!m_bChangeSettings && !m_bNewSeverity)
	{
		return true;
	}
	// Take control of the mutex before doing anything
	CMutexLock xLock(m_hLogMutex);
	if( !xLock.Lock(dwMilliseconds) )
	{
		TRACE_MSG( _T("WriteConfigFile - Failed to get mutex") );
		return false;
	}

	bool bRetVal = m_xLogConfigFile.WriteConfigFile(m_xLogData, m_xCategoryMap);
	if(bRetVal)
	{	// set the change settings flag to false
		m_bChangeSettings = false;
		m_bNewSeverity = false;
	}
	return bRetVal;	
}

//========================================================================

/* @name CCyclicLog::ReadConfigFileTime - read the last write time of the configuration file
** @param const DWORD dwMilliseconds - timeout option
** @return bool - true when file had changed, otherwise false */
bool CCyclicLog::ReadConfigFileTime(const DWORD dwMilliseconds)
{
	// Take control of the mutex before doing anything
	CMutexLock xLock(m_hLogMutex);
	if( !xLock.Lock(dwMilliseconds) )
	{
		TRACE_MSG( _T("ReadConfigFileTime - Failed to get mutex") );
		return false;
	}
	return m_xLogConfigFile.ReadConfigFileTime();
}

//========================================================================

/* @name CLogConfigFile::ReadConfigFile - read configuration from a config file (if exists)
** @param SLogData& xLogData -
** @param CategoryMap& xCategoryMap -
** @return bool  - */
bool CLogConfigFile::ReadConfigFile(SLogData&		xLogData,
									CategoryMap&	xCategoryMap)
{
	std::string strConfigName_string;
	tstring_helper::to_char(m_strFileName, strConfigName_string);
	tifstream ifConfig(strConfigName_string.c_str());
	if (!ifConfig.is_open())
	{	// failed to open the config log file
		return false;
	}

	long	lTemp = -1;
	size_t	nPos = tstring::npos;
	TCHAR	szBuffer[MAX_MSG_LENGTH] = { _T('\0') };
	bool	bSectionSettings = false;
	bool	bSectionCategory = false;

	// Scan the input until you find data which is not an empty line
	while (!ifConfig.eof())
	{
		ifConfig.getline(szBuffer, MAX_MSG_LENGTH);
		tstring strTemp(szBuffer);
		tstring_helper::trim_all(strTemp);
		if( !strTemp.length() )
		{	// ignore empty lines
			continue;
		}
		nPos = strTemp.find_first_of( _T("#") );
		if (0 == nPos)
		{	// ignore comments (the first character in line after trim is '#')
			continue;
		}

		// check if its a section looking for the '[' sign
		nPos = strTemp.find('[');
		if (tstring::npos != nPos)
		{	// [Settings] section
			nPos = strTemp.find(CLogConfigFile::s_szConfigSettings, ++nPos);
			if (tstring::npos != nPos)
			{
				bSectionSettings = true;
				bSectionCategory = false;
			}
			else
			{	// [Category] section
				nPos = strTemp.find(CLogConfigFile::s_szConfigCategory, ++nPos);
				if (tstring::npos != nPos)
				{
					bSectionSettings = false;
					bSectionCategory = true;
				}
				else
				{	// unknown section
					bSectionSettings = false;
					bSectionCategory = false;
				}
			}
			// after identifying the section read the next line 
			continue;
		}
		nPos = strTemp.find_first_of('=');
		if (tstring::npos != nPos)
		{
			tstring strKey(strTemp.c_str(), 0, nPos);
			tstring strValue(strTemp.c_str(), nPos+1, strTemp.size() );

			if(bSectionSettings)
			{
				if( 0==strKey.compare(CLogConfigFile::s_szConfigAppendOldFile) )
				{
					lTemp = _ttol( strValue.c_str() );
					xLogData.bAppendOldFile = lTemp ? true : false;
				}
				else if( 0==strKey.compare(CLogConfigFile::s_szConfigBinaryMode) )
				{
					lTemp = _ttol( strValue.c_str() );
					xLogData.bBinaryMode = lTemp ? true : false;
				}
				else if( 0==strKey.compare(CLogConfigFile::s_szConfigReportStdOut) )
				{
					lTemp = _ttol( strValue.c_str() );
					xLogData.bReportStdOut = lTemp ? true : false;
				}
				else if( 0==strKey.compare(CLogConfigFile::s_szConfigOutputDebugString) )
				{
					lTemp = _ttol( strValue.c_str() );
					xLogData.bOutputDebugString = lTemp ? true : false;
				}
				else if( 0==strKey.compare(CLogConfigFile::s_szConfigFileSize_MB) )
				{
					lTemp = _ttol( strValue.c_str() );
					xLogData.lFileSize = lTemp * ONE_MB;
				}
				else if( 0==strKey.compare(CLogConfigFile::s_szConfigNumOfFiles) )
				{
					lTemp = _ttol( strValue.c_str() );
					xLogData.nNumOfFiles = lTemp;
				}
				else if( 0==strKey.compare(CLogConfigFile::s_szConfigDelimiter) )
				{	// convert escape characters to whitespace characters
					nPos = strValue.find_first_of('\"');
					if (tstring::npos != nPos)
					{
						strValue.erase(0, ++nPos);
					}
					nPos = strValue.find_last_of('\"');
					if (tstring::npos != nPos)
					{
						strValue.erase(nPos, strValue.size());
					}
					tstring_helper::replace_all(strValue, _T("\\t"), _T("\t") );
					tstring_helper::replace_all(strValue, _T("\\n"), _T("\n") );
					tstring_helper::replace_all(strValue, _T("\\r"), _T("\r") );
					xLogData.strDelimiter = strValue;
				}
				else if( 0==strKey.compare(CLogConfigFile::s_szConfigLogType) )
				{
					lTemp = _ttol( strValue.c_str() );
					xLogData.eLogType = static_cast<LogTypes>(lTemp);
				}
				else if( 0==strKey.compare(CLogConfigFile::s_szConfigExtension) )
				{
					if( strValue.empty() )
					{	// Set extension according to log type if extension is empty
						switch(xLogData.eLogType)
						{
						case LogTypeText:
						case LogTypeTextNoCategory:
							strValue = _T(".log");
							break;
						case LogTypeMsgList:
						case LogTypeMsgRaw:
							strValue = _T(".txt");
							break;
						case LogTypeXml:
						case LogTypeLog4j:
							strValue = _T(".xml");
							break;
						case LogTypeHtmlLight:
						case LogTypeHtmlDark:
							strValue = _T(".html");
							break;
						}
					}
					xLogData.strLogExtension = strValue;
				}
				else if( 0==strKey.compare(CLogConfigFile::s_szConfigSeverityLevel) )
				{
					lTemp = _ttol( strValue.c_str() );
					xLogData.eSeverityLevel = CCyclicLog::_GetSeverityValue(lTemp);
				}
				else if( 0==strKey.compare(CLogConfigFile::s_szConfigCurrentFileNum) )
				{
					lTemp = _ttol( strValue.c_str() );
					xLogData.nCurrentFileNumber = lTemp;				
				}
			}
			else if(bSectionCategory)
			{	// set the section category only if its key is not empty
				if( strKey.length() )
				{
					lTemp = _ttol( strValue.c_str() );
					SeverityTypes eSeverityLevel = CCyclicLog::_GetSeverityValue(lTemp);
					if(	strKey.length() <= CATEGORY_LENGTH)
					{
						xCategoryMap[strKey] = eSeverityLevel;
					}
					else	// truncate the category length if needed
					{
						tstring strCategoryTrunc(strKey);
						strCategoryTrunc.erase(CATEGORY_LENGTH);
						xCategoryMap[strCategoryTrunc] = eSeverityLevel;
					}
				}
			}
 		}
	}	// end of while
	return true;
}

//========================================================================

/* @name CLogConfigFile::WriteConfigFile - write the configuration to a config file
** @param const SLogData& xLogData -
** @param const CategoryMap xCategoryMap - 
** @return bool  - */
bool CLogConfigFile::WriteConfigFile(const SLogData&	xLogData,
									 const CategoryMap&	xCategoryMap)
{
	std::string strConfigName_string;
	tstring_helper::to_char(m_strFileName, strConfigName_string);
	tofstream ofConfig(strConfigName_string.c_str());
	if (!ofConfig.is_open())
	{	// force change settings to write a new config file
		return false;
	}

	ofConfig << _T("######################### Configuration File Help #########################")
		_T("\n# Log File: ")  << xLogData.strLogFilePath << _T("\\") << xLogData.strLogFileName <<
		_T("\n###########################################################################")
		_T("\n# ")	<< CLogConfigFile::s_szConfigAppendOldFile
					<< _T(" - Append to old log file if exists (1=Append, 0=Replace)")
		_T("\n# ")	<< CLogConfigFile::s_szConfigBinaryMode
					<< _T(" - Open log file in binary mode (1=Binary, 0=Text)")
		_T("\n# ")	<< CLogConfigFile::s_szConfigReportStdOut
					<< _T(" - Report output to std::cout (1=On, 0=Off)")
		_T("\n# ")	<< CLogConfigFile::s_szConfigOutputDebugString
					<< _T(" - Report output to OutputDebugString (1=On, 0=Off)")
		_T("\n# ")	<< CLogConfigFile::s_szConfigFileSize_MB
					<< _T(" - Limited size for one File in MB (0=Unlimited)")
		_T("\n# ")	<< CLogConfigFile::s_szConfigNumOfFiles
					<< _T(" - Create a cyclic log files (0=No cyclic)")
		_T("\n# ")	<< CLogConfigFile::s_szConfigDelimiter
					<< _T(" - Delimiter text between fields (tab=\\t)")
		_T("\n# ")	<< CLogConfigFile::s_szConfigLogType
					<< _T(" - Type of the log:")
		_T("\n#				0=Text, 1=Xml, 2=Light Html, 3=Dark Html,")
		_T("\n#				4=Text messages only no header (include new line),")
		_T("\n#				5=Text raw messages only no header (no new line),")
		_T("\n#				6=Log4j XML log file,")
		_T("\n#				7=Text file without the category")
		_T("\n# ")	<< CLogConfigFile::s_szConfigExtension
					<< _T(" - File Extension including the dot ('.')")
		_T("\n# ")	<< CLogConfigFile::s_szConfigSeverityLevel
					<< _T(" - Minimum Severity to write:")
		_T("\n#				0=Debug,   1=Verbose, 2=Information,")
		_T("\n#				3=Warning, 4=Error,   5=Turned Off")
		_T("\n# ")	<< CLogConfigFile::s_szConfigCurrentFileNum
					<< _T(" - Current file number (for cycle mode)")
		_T("\n###########################################################################");

	WRITE_INI_HEADER(ofConfig, CLogConfigFile::s_szConfigSettings);
	WRITE_INI_PARAM(ofConfig, CLogConfigFile::s_szConfigAppendOldFile,		xLogData.bAppendOldFile);
	WRITE_INI_PARAM(ofConfig, CLogConfigFile::s_szConfigBinaryMode,			xLogData.bBinaryMode);
	WRITE_INI_PARAM(ofConfig, CLogConfigFile::s_szConfigReportStdOut,		xLogData.bReportStdOut);
	WRITE_INI_PARAM(ofConfig, CLogConfigFile::s_szConfigOutputDebugString,	xLogData.bOutputDebugString);
	// force 1MB when size is smaller than 1 MB
	long lSizeMB=xLogData.lFileSize / ONE_MB;
	if(0==lSizeMB)
	{
		lSizeMB = 1;
	}
	WRITE_INI_PARAM(ofConfig, CLogConfigFile::s_szConfigFileSize_MB,		lSizeMB);
	WRITE_INI_PARAM(ofConfig, CLogConfigFile::s_szConfigNumOfFiles,			xLogData.nNumOfFiles);

	// convert whitespace characters to escape characters
	tstring strDelimiter( _T("\"") );
	strDelimiter += xLogData.strDelimiter;
	strDelimiter += _T("\"");
	tstring_helper::replace_all(strDelimiter, _T("\t"), _T("\\t") );
	tstring_helper::replace_all(strDelimiter, _T("\n"), _T("\\n") );
	tstring_helper::replace_all(strDelimiter, _T("\r"), _T("\\r") );

	WRITE_INI_PARAM(ofConfig, CLogConfigFile::s_szConfigExtension,			xLogData.strLogExtension);
	WRITE_INI_PARAM(ofConfig, CLogConfigFile::s_szConfigDelimiter,			strDelimiter);
	WRITE_INI_PARAM(ofConfig, CLogConfigFile::s_szConfigLogType,			xLogData.eLogType);
	WRITE_INI_PARAM(ofConfig, CLogConfigFile::s_szConfigSeverityLevel,		xLogData.eSeverityLevel);
	if(0!=xLogData.nCurrentFileNumber)
	{	// update the current file number only if its not zeroed
		WRITE_INI_PARAM(ofConfig, CLogConfigFile::s_szConfigCurrentFileNum,	xLogData.nCurrentFileNumber);
	}

	// Write category section
	WRITE_INI_HEADER(ofConfig, CLogConfigFile::s_szConfigCategory);
	for(CategoryMap::const_iterator iter = xCategoryMap.begin();
		iter != xCategoryMap.end();
		iter++)
	{
		const tstring&			rCategory = iter->first;
		const SeverityTypes&	eSeverity = iter->second;
		WRITE_INI_PARAM(ofConfig, rCategory.c_str(), eSeverity);
	}
	ofConfig.close();
	// finally get the last write time of the configuration
	ReadConfigFileTime();
	return true;	
}

//========================================================================

/* @name CLogConfigFile::WriteConfigFile - write the severities configuration to a config file
** @param const CategoryMap& xCategoryMap -
** @return bool  - */
bool CLogConfigFile::WriteConfigFile(const CategoryMap& xCategoryMap)
{	// make sure the file exists
	std::string strConfigName_string;
	tstring_helper::to_char(m_strFileName, strConfigName_string);
	tifstream ifConfig(strConfigName_string.c_str());
	if (!ifConfig.is_open())
	{	// failed to open the config log file
		return false;
	}
	ifConfig.close();
	// Write category section
	BOOL bRetVal;
	TCHAR szBuffer[MAX_INT_LENGTH] = { _T('\0') };
	for(CategoryMap::const_iterator iter = xCategoryMap.begin();
		iter != xCategoryMap.end();
		iter++)
	{
		const tstring&			rCategory = iter->first;
		const SeverityTypes&	eSeverity = iter->second;

		_sntprintf_s(szBuffer, MAX_INT_LENGTH, _T("%d"), eSeverity);
		bRetVal = WritePrivateProfileString(CLogConfigFile::s_szConfigCategory,
											rCategory.c_str(), szBuffer,
											m_strFileName.c_str() );
		if(!bRetVal)
		{
			break;
		}
	}
	return (bRetVal==TRUE);
}

//========================================================================

/* @name CLogConfigFile::ReadConfigFileTime - read the last write time of the configuration file
** @return bool - true when file had changed, otherwise false */
bool CLogConfigFile::ReadConfigFileTime()
{
	bool bRetVal = false;
	HANDLE hFile = ::CreateFile(m_strFileName.c_str(),
								GENERIC_READ, FILE_SHARE_READ, NULL,
								OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE != hFile)
	{
		FILETIME ftLastWrite;
		GetFileTime(hFile, NULL, NULL, &ftLastWrite);
		CloseHandle(hFile);
		if ( !CompareTime(ftLastWrite) )
		{	// file time had changed - keep it and return true
			m_ftLastWrite = ftLastWrite;
			bRetVal = true;
		}
	}
	return bRetVal;
}

//========================================================================
// MACRO - undefine all local macros
//========================================================================
#undef LOG4J_END_LINE
#undef XML_END_LINE
#undef XML_EOF
#undef XML_HEADER

#undef HTML_SEPARATOR
#undef HTML_END_LINE
#undef HTML_EOF
#undef HTML_HEADER

#undef _TRACE_MSG
#undef TRACE_MSG
#undef TRACE_ERROR

#undef SAFE_CLOSE_HANDLE
#undef CHECK_FILE_RETURN_FALSE

#undef WRITE_INI_HEADER
#undef WRITE_INI_PARAM
