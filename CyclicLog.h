//////////////////////////////////////////////////////////////////////
// CyclicLog.h: interface for the CCyclicLog class.
//////////////////////////////////////////////////////////////////////
// Created by Ilan Gavriel 
//////////////////////////////////////////////////////////////////////
// Initialize the log using one of the following constructors:
//--------------------------------------------------------------------
//	1)	Logger::SLogData	xLogData;
//		xLogData.strLogFileName = _T("FileName");
//		xLogData.nNumOfFiles = 3;
//		xLogData.strDelimiter = _T("\t|\t");
//			:
//		Logger::CCyclicLog Log(xLogData);
//
//	2)	Logger::CCyclicLog Log("LogFileName");
//			in this case the log path will be the current running path
//
//	3)	Logger::CCyclicLog Log("LogFileName", "D:\\LogPath");
//
//--------------------------------------------------------------------
// Log Events:
//--------------------------------------------------------------------
//	Log.LogEvent(L_ERROR, "Category", "Error Number %d", rand() );
//
//	Log.LogEvent(L_WARN,  _T("Module"), _T("Warning: %d"), rand() );
//
//	Log.LogEvent(L_INFO,  NULL, "Important information");
//
//	Log.LogEvent("Message without category and file location", S_INFO);
//
//	if( Log.IsEffective(S_VERB, "Module") )
//		Log.LogEvent(L_VERB,  "Module", "Verbose Message %d",  rand() );
//
//	if( Log.IsEffective(Logger::SeverityDebug, _T("Category") )
//		Log.LogEvent(L_DEBUG, _T("Category"), "Debug Message %d",  rand() );
//
//////////////////////////////////////////////////////////////////////
// History:
// 24/08/2008 - VS2008 fixes
// 				This version contains the ability of types for the log 
// 				we have 4 types TXT (CSV or any other delimiter), XML, and HTML
// 				This version can limit the number of files and cycle them 
//
// 17/03/2009 - Add Unicode / MBCS support using TCHAR
//
// 19/03/2009 - namespace CyclicLog
//				Report to std::cout with colors according to configuration
//				use std::exception and not TCHAR*
//
// 22/03/2009 - Fix Spelling Mistakes
//				Add new SLogMsg for event messages struct
//				Use SYSTEMTIME as timestamp
//				Improve Init method
//				Add CritSec internally to the log file object
//				Add private method: InitializeMutex for mutex startup
//
// 23/03/2009 - Add a internal Thread to Cyclic Log
//				Stop using CCritSec Class
//
// 24/03/2009 - Add combined levels macros
//				Add method that supports vargs(...)
//
// 25/03/2009 - Always flush file,
//				Add Accessors for SLogMsg struct
//				Set to static: _IsEffective, _SetSeverityLevel, _GetCategoryMap
//				Severities map have changed to static vectors with pointer to them
//				Mutex have been change to static as well: s_hLogMutex
//				Adding sleep in unlock method when mutex is not available
//
// 26/03/2009 - Rollback all static issues - to allow multiple logs in the same application
//				Support wchar_t File Name
//
// 30/03/2009 - Delete the old m_queueMsgs (string based) and use the new
//					LogMsgQueue, the format process of the log message will be
//					done on the thread context
//				Rename the namespace from CyclicLog to Logger
//				On console mode, change console title to the log file name
//				Add helper macros:	TRACE_MSG, TRACE_ERROR,
//									CHECK_LOG_FILE_RETURN_FALSE,
//									SAFE_CLOSE_HANDLE
//				The method InitializeMutex() - create a suspended thread and
//					the Init() method resume the thread
//				Support ReadConfigFile/WriteConfigFile
//				Adding tstring_helper namespace with helper functions
//
// 06/04/2009 - move the tstring declaration outside the class to a separate file
//				Limit the category length to maximum size of CATEGORY_LENGTH
//				WriteConfigFile - write the ini file explicitly
//				Add debug lines [DEBUG][Locktime] to measure lock time
//					unmark those lines in needed - measured lock time was about 10 ms
//
// 07/04/2009 - Implement the ReadConfigFile method
//				WriteCurrentFileNumber to the new configuration ini file
//				ReadConfigFileTime - for change settings
//				Bug Fix: IsEffective and ReadConfigFile should SetSeverityLevel
//					for new categories only when the the key is not empty
//
// 19/04/2009 - Report OutputDebugString if enabled
//				Skip reading config file if it didn't changed
//				CLogConfigFile - new class to handle config files
//				Bug Fix: Use global severity only when needed in SetSeverityLevel
//
// 27/04/2009 - Bug Fix: Access violation when initializing static stl::maps
//				Improvement: Add the prefix "Log_" to the mutex name to
//					allow unique log mutex name
//
// 24/05/2009 - Implement Config Thread to track changes of configuration
//				during runtime
//
// 25/05/2010 - Add Method: WriteConfigFile to write the severities configuration
//					to a config file (used by the config dialog)
//				When new severity is added to the map, save it to the 
//					log file as well
//
// 13/09/2010 - Add reference count for mutex in a static map of mutex name to 
//					counter, in order to support only once handle close for the mutex
//
// 14/02/2011 - Support log4j output format
//
// 05/06/2013 - Replace all _sntprintf with _sntprintf_s
//				Bug fix: when Failed to read to config file - force change settings 
//						to write a new config file
//
// 07/08/2013 - Add new log type without category field LogTypeTextNoCategory=7
//
// 29/08/2013 - support LogEventV method to pass multiple parameters using va_list
//
// 06/01/2014 - Support CyclicLog.xsl when using XML mode
//				Set extension from INI file or using default according to log type 
//					if extension is empty
//
//////////////////////////////////////////////////////////////////////

#ifndef __CyclicLog_H__
#define __CyclicLog_H__

#if ( defined(_UNICODE) && !defined(UNICODE) )
	#error Logger::CCyclicLog: You need to define UNICODE preprocessor as well
#endif

#pragma warning (disable:4786)

#include <tstring.hpp>
#include <queue>
#include <map>
#include <assert.h>
#include <time.h>

//========================================================================
// MACRO - predefined for file location and level severity
//========================================================================
#ifdef _UNICODE
	// convert ansi char to wchar_t
	#define TO_WIDE2(x)	L ## x
	#define TO_WIDE(x)	TO_WIDE2(x)
	// convert ansi file name to wchar_t file name
	#define __WFILE__	TO_WIDE(__FILE__)
	// location is line and wchar_t file name
	#define __TFILE__	__WFILE__
#else
	// location is line and ansi file name
	#define __TFILE__	__FILE__
#endif

#define LOG_LOCATION	__LINE__, __TFILE__

// predefined severities and location macros for the LogEvent method use 
// SLOG_XXX - used by the IsEffective method
// PLOG_XXX - used by the LogEvent method
#define SLOG_OFF		Logger::SeverityOff
#define SLOG_ERROR		Logger::SeverityError
#define SLOG_WARN		Logger::SeverityWarning
#define SLOG_INFO		Logger::SeverityInfo
#define SLOG_VERB		Logger::SeverityVerbose
#define SLOG_DEBUG		Logger::SeverityDebug

#define PLOG_ERROR		SLOG_ERROR,	LOG_LOCATION
#define PLOG_WARN		SLOG_WARN,	LOG_LOCATION
#define PLOG_INFO		SLOG_INFO,	LOG_LOCATION
#define PLOG_VERB		SLOG_VERB,	LOG_LOCATION
#define PLOG_DEBUG		SLOG_DEBUG,	LOG_LOCATION

//========================================================================
// namespace Logger
//========================================================================
namespace Logger
{

//========================================================================
// constant
//========================================================================

static const unsigned int TIME_LENGTH		= 30;	// for the time format string
static const unsigned int STATUS_LENGTH		= 3;	// for the file open format
static const unsigned int CATEGORY_LENGTH	= 15;	// maximum category length
static const unsigned int MAX_INT_LENGTH	= 32;	// maximum integer length
static const unsigned int MAX_MSG_LENGTH	= 2048;	// maximum log line

static const unsigned int ONE_MB			= 1 * 1024 * 1024;
static const unsigned int ONE_SEC			= 1 * 1000;
static const unsigned int MAX_LOCK_TIMEOUT	= 3 * ONE_SEC;
static const unsigned int DESTRUCT_TIMEOUT	= 5 * ONE_SEC;

//========================================================================
// enumeration / typedef
//========================================================================

typedef enum ELogTypes {
	LogTypeText = 0,	// Text log file 
	LogTypeXml,			// XML log file 
	LogTypeHtmlLight,	// HTML (light) log file 
	LogTypeHtmlDark,	// HTML (dark) log file 
	LogTypeMsgList,		// Text log file - Messages only no header (include new line)
	LogTypeMsgRaw,		// Text log file - Messages only no header (no new line)
	LogTypeLog4j,		// Log4j XML log file
	LogTypeTextNoCategory,	// Text log file without the category field
} LogTypes;

//========================================================================

typedef enum ESeverityTypes {
	Severity_FIRST = 0,	//indicator for lower value
	//--------------------------------------------------------------------
	SeverityDebug	= Severity_FIRST,	// Debug
	SeverityVerbose,					// Verbose
	SeverityInfo,						// Information
	SeverityWarning,					// Warning
	SeverityError,						// Error
	//--------------------------------------------------------------------
	SeverityOff,						// Turned off
	//--------------------------------------------------------------------
	Severity_LAST = SeverityOff	//indicator for upper value
} SeverityTypes;

//========================================================================

typedef struct LogColors
{
	SeverityTypes	eSeverity;
	WORD			wAttributes;        // text and background colors
} SLogColors;

static LogColors LogColorsMap[] =
{
	{ SeverityDebug,	0x0F},	// WHITE     
	{ SeverityVerbose,	0x07},	// GRAY      
	{ SeverityInfo,		0x0A},	// GREEN     
	{ SeverityWarning,	0x0D},	// PURPLE    
	{ SeverityError,	0x0C}	// RED       
};

//========================================================================

//========================================================================
// Use this struct to init the CCyclicLog
//========================================================================
typedef struct LogData
{
public:
	LogData()
		: bAppendOldFile(false),
			bBinaryMode(false),
			bReportStdOut(false),
			bOutputDebugString(false),
			lFileSize(ONE_MB),
			nNumOfFiles(0),
			strLogFileName( _T("") ),
			strLogFilePath( _T("") ),
			strDelimiter( _T("\t") ),
			eLogType(LogTypeText),
			strLogExtension( _T("") ),
			eSeverityLevel(SeverityInfo),
			nCurrentFileNumber(1)
	{}
	//--------------------------------------------------------------------
	bool			bAppendOldFile;		// Append to Old Log file if exists
	bool			bBinaryMode;		// Open Log File in Binary Mode
	bool			bReportStdOut;		// report output to std::cout
	bool			bOutputDebugString;	// report output to OutputDebugString
	unsigned long	lFileSize;			// Limited size for one File (0 = Unlimited)
	unsigned int	nNumOfFiles;		// Create a cyclic log files (0 = No cyclic)
	tstring			strLogFileName;		// File Name (without Extension) - not saved in the INI
	tstring			strLogFilePath;		// File Path - not saved in the INI
	tstring			strDelimiter;		// Delimiter text between fields (default=tab)
	LogTypes		eLogType;			// Type of the log
	tstring			strLogExtension;	// File Extension - must be declared after the log type!!!
	SeverityTypes	eSeverityLevel;		// Minimum Severity to write from
	unsigned int	nCurrentFileNumber;	// Current file number
} SLogData;

//========================================================================
// Use this struct to keep Log Messages
//========================================================================
typedef struct LogMsg
{
public:
	LogMsg(const TCHAR*	szMessage,
		   const int	nSeverityLevel = SeverityInfo,
		   const TCHAR*	szCategory = _T(""),
		   const TCHAR*	szFileName = _T(""),
		   const unsigned int nLine = 0)
		: strMessage(szMessage),
			nSeverity(nSeverityLevel),
			strCategory(szCategory),
			strFileName(szFileName),
			nLineNumber(nLine)
	{
		dwProcessId = GetCurrentProcessId();
		dwThreadId = GetCurrentThreadId();
		GetLocalTime(&tTimeStamp);		// Current Local time
		// limit the category length to maximum size of CATEGORY_LENGTH
		if(	strCategory.length() > CATEGORY_LENGTH)
		{
			strCategory.erase(CATEGORY_LENGTH);
		}
	}

    tstring ProcessId(bool bHex=false) const
	{
		TCHAR szBuffer[MAX_INT_LENGTH] = { _T('\0') };
		_sntprintf_s(szBuffer, MAX_INT_LENGTH,
					bHex ? _T("0x%04x") : _T("%04d"),
					dwProcessId);
	    return ( tstring(szBuffer) );
	}

    tstring ThreadId(bool bHex=false) const
	{
		TCHAR szBuffer[MAX_INT_LENGTH] = { _T('\0') };
		_sntprintf_s(szBuffer, MAX_INT_LENGTH,
					bHex ? _T("0x%04x") : _T("%04d"),
					dwThreadId);
	    return ( tstring(szBuffer) );
	}

	tstring TimeStamp() const
	{
		TCHAR szBuffer[TIME_LENGTH] = { _T('\0') };
		_sntprintf_s(szBuffer, TIME_LENGTH, _T("%02d/%02d/%04d %02d:%02d:%02d.%03d"),
					tTimeStamp.wDay,
					tTimeStamp.wMonth,
					tTimeStamp.wYear,
					tTimeStamp.wHour,
					tTimeStamp.wMinute,
					tTimeStamp.wSecond,
					tTimeStamp.wMilliseconds);
	    return ( tstring(szBuffer) );
	}

	operator time_t() const 
	{
		struct tm xTm = {0};
        xTm.tm_sec = 	tTimeStamp.wSecond;			// seconds after the minute - [0,59]
		xTm.tm_min = 	tTimeStamp.wMinute;			// minutes after the hour - [0,59]
        xTm.tm_hour = 	tTimeStamp.wHour;			// hours since midnight - [0,23]
		xTm.tm_mday = 	tTimeStamp.wDay;			// day of the month - [1,31]
        xTm.tm_mon = 	tTimeStamp.wMonth - 1;		// months since January - [0,11]
		xTm.tm_year = 	tTimeStamp.wYear - 1900;	// years since 1900
		xTm.tm_wday = 	tTimeStamp.wDayOfWeek;
		return mktime(&xTm);
	}

	tstring LineNumber() const
	{
		TCHAR szBuffer[MAX_INT_LENGTH] = { _T('\0') };
		_sntprintf_s(szBuffer, MAX_INT_LENGTH, _T("%u") ,dwThreadId);
	    return ( tstring(szBuffer) );
	}

	//--------------------------------------------------------------------
	DWORD			dwProcessId;		// Current Process Id
	DWORD			dwThreadId;			// Current Thread Id
	tstring			strMessage;			// Event Message
	tstring			strCategory;		// Event Category
	int				nSeverity;			// Event Severity
	SYSTEMTIME		tTimeStamp;			// Event Timestamp
	tstring			strFileName;		// File Name
	unsigned int	nLineNumber;		// Line Number
} SLogMsg;

//========================================================================
// Typedef
//========================================================================
typedef std::map<tstring, SeverityTypes>	CategoryMap;
typedef std::queue<SLogMsg>					LogMsgQueue;
typedef std::vector<tstring>				StringVector;

//========================================================================
// CLogConfigFile
//========================================================================
class CLogConfigFile
{
public:
	CLogConfigFile()
	{
		m_ftLastWrite.dwLowDateTime = 0;
		m_ftLastWrite.dwHighDateTime = 0;
	}

	// set config file name - strFileName = full path to config file name
	void Init(const tstring& strFileName)	{ m_strFileName = strFileName; }
	const TCHAR* GetFileName() const		{ return m_strFileName.c_str(); }

	bool CompareTime(FILETIME& ftTime)
	{
		return (m_ftLastWrite.dwLowDateTime  == ftTime.dwLowDateTime &&
				m_ftLastWrite.dwHighDateTime == ftTime.dwHighDateTime);
	}

	// read configuration from a config file (if exists)
	bool ReadConfigFile(SLogData&			xLogData,
						CategoryMap&		xCategoryMap);

	// write the configuration to a config file
	bool WriteConfigFile(const SLogData&	xLogData,
						 const CategoryMap&	xCategoryMap);

	// write the severities configuration to a config file
	bool WriteConfigFile(const CategoryMap&	xCategoryMap);

	// read the last write time of the configuration file
	// return true when file had changed, otherwise false 
	bool ReadConfigFileTime();

private:
	// hidden copy constructor and copy assignment
	CLogConfigFile(const CLogConfigFile& rOther);
	CLogConfigFile& operator=(const CLogConfigFile& rOther);

private:
	FILETIME	m_ftLastWrite;
	tstring		m_strFileName;

//========================================================================
//	static member declaration
//========================================================================
public:
	// Static strings for configuration file
	static const TCHAR		s_szConfigSettings[];
	static const TCHAR		s_szConfigCategory[];
	static const TCHAR		s_szConfigAppendOldFile[];
	static const TCHAR		s_szConfigBinaryMode[];
	static const TCHAR		s_szConfigReportStdOut[];
	static const TCHAR		s_szConfigOutputDebugString[];
	static const TCHAR		s_szConfigFileSize_MB[];
	static const TCHAR		s_szConfigNumOfFiles[];
	static const TCHAR		s_szConfigDelimiter[];
	static const TCHAR		s_szConfigLogType[];
	static const TCHAR		s_szConfigExtension[];
	static const TCHAR		s_szConfigSeverityLevel[];
	static const TCHAR		s_szConfigCurrentFileNum[];
};

//========================================================================
// CCyclicLog
//========================================================================
class CCyclicLog  
{
public:
	// Construction/Destruction:
	CCyclicLog(const SLogData&	xLogData);					/* throw(std::exception) */

	CCyclicLog(const TCHAR*		szLogFileName,
			   const TCHAR*		szLogFilePath = _T("") );	/* throw(std::exception) */

	virtual ~CCyclicLog();

	// Write an Event to the Log File
	bool LogEvent(const SLogMsg& xLogMsg);
	
    bool LogEvent(const int		nSeverity,
                  const int		nLineNumber,
				  const TCHAR*	szFileName,
                  const TCHAR*	szCategory,
                  const TCHAR*	szLogMsgFormat, ...);

	bool LogEventV(const int	nSeverity,
                  const int		nLineNumber,
				  const TCHAR*	szFileName,
                  const TCHAR*	szCategory,
				  const TCHAR*	szLogMsgFormat,
                  const va_list	args);

	bool LogEvent(const TCHAR*	szLogMsg,
				  const int		nSeverity =		SeverityInfo,
				  const TCHAR*	szCategory =	_T(""),
				  const TCHAR*	szFileName =	_T(""),
				  const int		nLineNumber =	0);

	// Check if Severity Level is effective
	bool IsEffective(const SLogMsg& xLogMsg);
	bool IsEffective(const int		nSeverityLevel, const tstring& strCategory);
	bool IsEffective(const int		nSeverityLevel = SeverityInfo,
					 const TCHAR*	szCategory = NULL);

	// Set The Minimal Severity Level, which the Log Object will accept
	void SetSeverityLevel(const int		nSeverityLevel, const tstring& strCategory);
	void SetSeverityLevel(const int		nSeverityLevel = SeverityInfo,
						  const TCHAR*	szCategory = NULL);

	// Get a copy of the entire Category Map
	bool GetCategoryMap(CategoryMap& rCategoryMap) const;

	// Force file rotation NOW (with timeout option)
	bool RotateLogFile(const DWORD dwMilliseconds = ONE_SEC);

	// Active/Deactive the Log Object
	inline void ActivateLog(const bool bStatus = true)		{ m_bActive = bStatus; }

	// Change the Limit size for one Log File
	inline bool ChangeLogFileSize(const long& lFileSize)	{ m_xLogData.lFileSize = lFileSize; }

	// Get the severity enum value from the given integer
	inline static SeverityTypes _GetSeverityValue(const int nSeverity)
	{
		if (nSeverity<Severity_FIRST) {
			return Severity_FIRST;
		} else if (nSeverity>Severity_LAST) {
			return Severity_LAST;
		}
		return static_cast<SeverityTypes>(nSeverity);
	}

protected:
	// initialization method
	virtual bool	Init(); /* throw(std::exception) */

	// The Actual Writing to File Action
	virtual bool	WriteMsgToFile(const SLogMsg& xLogMsg);
	virtual bool	WriteMsgToFile(const TCHAR* szMsg);

	// Gets a number and returns the Severity Text
	tstring		GetSeverity(const int nSeverity) const
	{
		assert(m_pCurrentSeverities);
		SeverityTypes eSeverity = CCyclicLog::_GetSeverityValue(nSeverity);
		return (*m_pCurrentSeverities)[eSeverity];
	}

	// The Rename The File Name
	bool		RenameFileName();

	// The Actual Rotate Action
	bool		RotateAction();

	// read configuration from a config file (if exists)
	bool		ReadConfigFile(const DWORD dwMilliseconds = ONE_SEC);

	// write the configuration to a config file
	bool		WriteConfigFile(const DWORD dwMilliseconds = ONE_SEC);

	// read the last write time of the configuration file
	// return true when file had changed, otherwise false 
	bool		ReadConfigFileTime(const DWORD dwMilliseconds = ONE_SEC);

	// Write the current number of file we use to the configuration file
	bool		WriteCurrentFileNumber(const DWORD dwMilliseconds = ONE_SEC);

	// Add XML/HTML header to the file 
	bool		WriteHeaderToFile(const TCHAR* szMsg);

	// Get XML/HTML properties
	tstring		GetLogProperties(const int nSeverity) const
	{
		assert(m_pCurrentSeveritiesPrefix);
		SeverityTypes eSeverity = CCyclicLog::_GetSeverityValue(nSeverity);
		return (*m_pCurrentSeveritiesPrefix)[eSeverity];
	}

private:
	// hidden copy constructor and copy assignment
	CCyclicLog();
	CCyclicLog(const CCyclicLog& rOther);
	CCyclicLog& operator=(const CCyclicLog& rOther);

	// mutex and thread initialization
	bool			InitializeMutex();	/* throw(std::exception) */

	// Initialize all static severities map
	static void		_InitializeSeverities();

	// Windows Function and a member for thread
	static DWORD WINAPI _LogThread(LPVOID pObject);
	static DWORD WINAPI _ConfigThread(LPVOID pObject);

	// The Main Thread Function which will used by the thread
	void			RunLogThread();
	void			RunConfigThread();

//========================================================================
//	member declaration
//========================================================================
private:
	HANDLE			m_hLogMutex;	// protect m_pLogFile and s_xCategoryMap

	BOOL			m_bMutexAlreadyExists;

	DWORD			m_dwLogThreadId;
	DWORD			m_dwConfigThreadId;
	HANDLE			m_hLogThread;
	HANDLE			m_hConfigThread;

	HANDLE			m_hQuitEvent;
	HANDLE			m_hNewLogEvent;

protected:
	LogMsgQueue		m_xLogMessagesQueue;
	SLogData		m_xLogData;
	FILE*			m_pLogFile;
	StringVector*	m_pCurrentSeverities;
	StringVector*	m_pCurrentSeveritiesPrefix;
	CategoryMap		m_xCategoryMap;
	CLogConfigFile	m_xLogConfigFile;

	bool			m_bActive;
	bool			m_bCycle;
	bool			m_bChangeSettings;
	bool			m_bNewSeverity;

	TCHAR			m_cFileStatus[STATUS_LENGTH];

	tstring			m_strDelimiter;
	tstring			m_strFileName;
	tstring			m_strExtension;
	tstring			m_strAbsoluteFileName;
	tstring			m_strMutexName;

//========================================================================
//	static member declaration
//========================================================================
protected:
	// Static String vectors for severities
	static StringVector		s_xSeverities;
	static StringVector		s_xSeveritiesXml;
	static StringVector		s_xSeveritiesXmlPrefix;
	static StringVector		s_xSeveritiesHtmlDarkPrefix;
	static StringVector		s_xSeveritiesHtmlLightPrefix;
	static StringVector		s_xSeveritiesLog4jPrefix;
};

//========================================================================
// CMutexLock - based on MFC CSingleLock and CSyncObject 
// taken from source files:
//	"C:\Program Files\Microsoft Visual Studio 10.0\VC\atlmfc\include\afxmt.h"
//	"C:\Program Files\Microsoft Visual Studio 10.0\VC\atlmfc\include\afxmt.inl"
//	"C:\Program Files\Microsoft Visual Studio 10.0\VC\atlmfc\src\mfc\mtcore.cpp"
//	"C:\Program Files\Microsoft Visual Studio 10.0\VC\atlmfc\src\mfc\mtex.cpp"
//========================================================================
class CMutexLock
{
public:
	// Constructor/Destructor
	CMutexLock(HANDLE hMutex, bool bInitialLock = false)
	{
		assert(hMutex != NULL);
		m_hMutex = hMutex;
		m_bAcquired = false;
		if (bInitialLock)
		{
			Lock();
		}
	}
	virtual ~CMutexLock() { Unlock(); }

	// accessor
 	bool IsLocked()	const {  return m_bAcquired; }

	// Operations
	bool Lock(DWORD dwMilliseconds = INFINITE)
	{
		assert(m_hMutex != NULL);
		assert(!m_bAcquired);
//*[DEBUG][Locktime]*/	m_dwLockTime = GetTickCount();	// Get tick time before lock
		DWORD dwResult = ::WaitForSingleObject(m_hMutex, dwMilliseconds);
		if (dwResult == WAIT_OBJECT_0 || dwResult == WAIT_ABANDONED)
		{
			m_bAcquired = true;
		}
		else
		{
			m_bAcquired = false;
		}
		return m_bAcquired;
	}

	bool Unlock()
	{
		assert(m_hMutex != NULL);
		if (m_bAcquired)
		{
			m_bAcquired = !ReleaseMutex(m_hMutex);
		}
		else
		{	// in case it was not acquired, switch context
			Sleep(0);
		}
//*[DEBUG][Locktime]*/	m_dwLockTime = GetTickCount() - m_dwLockTime;	// Calculate lock time in ticks after unlock

		// successfully unlocking means it isn't acquired
		return !m_bAcquired;
	}

protected:
	HANDLE 	m_hMutex;
	bool   	m_bAcquired;
//*[DEBUG][Locktime]*/	DWORD	m_dwLockTime;	// lock time measurement in ticks
};

};	// namespace Logger

#endif //__CyclicLog_H__