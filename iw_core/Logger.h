/*
*	The Altalena Project File
*	Copyright (C) 2009  Boris Ouretskey
*
*	This library is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*
*	This library is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*
*	You should have received a copy of the GNU Lesser General Public
*	License along with this library; if not, write to the Free Software
*	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#pragma once

using namespace boost;
using namespace csp;
using namespace std;
using namespace JadedHoboConsole;
namespace con = JadedHoboConsole;

#include "Configuration.h"
#include "DllHelpers.h"

namespace ivrworx
{
	
	string IW_CORE_API FormatLastSysError(const char *lpszFunction);

	BOOL IW_CORE_API InitLog(ConfigurationPtr conf);	

	void IW_CORE_API ExitLog();

	#define IW_LOG_MASK_CONSOLE		0x0001
	#define IW_LOG_MASK_DEBUGVIEW	0x0010
	#define IW_LOG_MASK_SYSLOG		0x0100
	#define IW_LOG_MASK_FILE		0x1000

	void
	SetLogMask(IN int mask);

	enum LogLevel
	{
		LOG_LEVEL_OFF,
		LOG_LEVEL_CRITICAL,
		LOG_LEVEL_WARN,
		LOG_LEVEL_INFO,
		LOG_LEVEL_DEBUG,
		LOG_LEVEL_TRACE
	};

	extern IW_CORE_API LogLevel g_MaxLogLevel;

	void
	SetLogLevelFromString(const string &level_str);

	void
	SetLogMaskFromString(const string &mask_str);

	void
	SetLogLevel(IN LogLevel log_level);

	IW_CORE_API void 
	IwStartScript();
	
	IW_CORE_API void
	IwStopScript();
	
	typedef 
	std::basic_stringbuf<char, std::char_traits<char>>	char_string_buf;

	class IW_CORE_API basic_debugbuf: 
		public char_string_buf
	{
	public:
		~basic_debugbuf();
	protected:
		int sync();
	public:
		LogLevel log_level;
	};

	typedef
	std::basic_ostream<char, std::char_traits<char>> char_stream;

	class IW_CORE_API debug_dostream: 
		public char_stream
	{
	public:
		debug_dostream();
		~debug_dostream(); 

		inline void set_log_level(LogLevel log_level)
		{
			((basic_debugbuf*)rdbuf())->log_level = log_level;
		}

	};

	extern __declspec( thread ) debug_dostream *tls_logger;

	class IW_CORE_API LoggerTracker
	{
	public:
		LoggerTracker(char *log_function);
		~LoggerTracker();
	private:
		static const int MAX_LENGTH = 100; 
		char _funcname[MAX_LENGTH];
	};

	#define __STR2WSTR(str)    L##str
	#define _STR2WSTR(str)     __STR2WSTR(str)

	#define __FILEW__          _STR2WSTR(__FILE__)
	#define __FUNCTIONW__      _STR2WSTR(__FUNCTION__)

	#define TLS_LOGGER_SLOT 10

	IW_CORE_API debug_dostream *GetTlsLogger();

	#define IX_SCOPED_LOG(level,x) {			\
		debug_dostream *tls_logger = GetTlsLogger();\
		(*tls_logger).set_log_level(level);		\
		(*tls_logger) << x << std::endl;		\
	}	

	#ifdef PROFILE
		#define LogProfile(x)	IX_SCOPED_LOG(LOG_LEVEL_DEBUG,x)
	#else
		#define LogProfile(x)
	#endif

	#define COND_LOG(level,x) if (::InterlockedExchangeAdd(( LONG *)&g_MaxLogLevel,0) >= level) IX_SCOPED_LOG(level,x)

#ifndef NOLOGS
	// hack to display script logging 
	// in different color
	#define LogStartScriptLog()		IwStartScript()
	#define LogStopScriptLog()	IwStopScript()

	#define LogDebug(x)		COND_LOG(LOG_LEVEL_DEBUG,x)
	#define LogInfo(x)		COND_LOG(LOG_LEVEL_INFO,x)
	#define LogWarn(x)		COND_LOG(LOG_LEVEL_WARN,x)
	#define LogCrit(x)		COND_LOG(LOG_LEVEL_CRITICAL,x)
	#define LogSysError(x) 	COND_LOG(LOG_LEVEL_WARN, FormatLastSysError(x))

#ifdef TRACE_FUNC_CALLS
	#define LogTrace(x)		COND_LOG(LOG_LEVEL_TRACE,x)
	#define FUNCTRACKER LoggerTracker _ltTag(__FUNCTION__) 
#else 
	#define LogTrace(x)		
	#define FUNCTRACKER 
#endif

#else
	#define LogStartScriptLog()		
	#define LogStopScriptLog()
	#define LogTrace(x) 
	#define LogDebug(x) 
	#define LogInfo(x)  
	#define LogWarn(x)  
	#define LogCrit(x)	

	#define LogTraceRaw(x) 
	#define LogDebugRaw(x) 
	#define LogInfoRaw(x)  
	#define LogWarnRaw(x)  
	#define LogCritRaw(x)

	#define LogSysError(x)

	#define FUNCTRACKER  
#endif

extern ostream& operator << (ostream& os, const Time &time);

}