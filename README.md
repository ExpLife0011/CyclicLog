# CyclicLog

#### Initialization
Initialize the log using one of the following constructors:

```
Logger::SLogData	xLogData;
xLogData.strLogFileName = _T("FileName");
xLogData.nNumOfFiles = 3;
xLogData.strDelimiter = _T("\t|\t");
		:
Logger::CCyclicLog Log(xLogData);
```

```
Logger::CCyclicLog Log("LogFileName"); // in this case the log path will be the current running path
```

```
Logger::CCyclicLog Log("LogFileName", "D:\\LogPath");
```

#### Log Events:
```
Log.LogEvent(L_ERROR, "Category", "Error Number %d", rand() );
```

```
Log.LogEvent(L_WARN,  _T("Module"), _T("Warning: %d"), rand() );
````

```
Log.LogEvent(L_INFO,  NULL, "Important information");
```

```
Log.LogEvent("Message without category and file location", S_INFO);
```

```
if( Log.IsEffective(S_VERB, "Module") )
		Log.LogEvent(L_VERB,  "Module", "Verbose Message %d",  rand() );
```

```
if( Log.IsEffective(Logger::SeverityDebug, _T("Category") )
		Log.LogEvent(L_DEBUG, _T("Category"), "Debug Message %d",  rand() );
```
