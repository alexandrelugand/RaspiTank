#include <stdio.h>
#include <stdarg.h>
#include <libgen.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>
#include "log.h"
#include "WebSocketServer.h"
#include <string>
#include "Utils.h"

using namespace std;
using namespace RaspiTank;

int log_flags=0;
static pthread_mutex_t log_mutex=PTHREAD_MUTEX_INITIALIZER;
#ifdef __DEBUG__
static const char *debug0=NULL;
#endif

void log_syslog(log_level level, const char *filename, int lineno, const char *fmt, ...);
void log_stderr(log_level level, const char *filename, int lineno, const char *fmt, ...);

/**
 * @short Logs a message to the log facility.
 * 
 * Normally to stderr, but can be set to your own logger or to use syslog.
 * 
 * It is actually a variable which you can set to any other log facility. By default it is to 
 * log_stderr, but also available is log_syslog.
 * 
 * @param level Level of log. 
 * @param filename File where the message appeared. Usefull on debug level, but available on all.
 * @param lineno Line where the message was produced.
 * @param fmt printf-like format string and parameters.
 */
void (*trace_log)(log_level level, const char *filename, int lineno, const char *fmt, ...)=log_stderr;

/**
 * @short Logs to stderr.
 * 
 * It can be affected also by the environment variable TRACE_LOG, with one or several of:
 * 
 * - "nocolor"  -- then output will be without colors.
 * - "syslog"   -- Switchs the logging to syslog. 
 * - "noinfo"   -- Do not show info lines.
 * - "nodebug"  -- When in debug mode, do not show debug lines.
 * 
 * Also for DEBUG0 level, it must be explictly set with the environmental variable TRACE_DEBUG0, set
 * to the names of the files to allow DEBUG0 debug info. For example:
 * 
 *   export TRACE_DEBUG0="url.c"
 * 
 * It is thread safe.
 * 
 * When compiled in release mode (no __DEBUG__ defined), DEBUG and DEBUG0 are not compiled so they do
 * not incurr in any performance penalty.
 */
void log_stderr(log_level level, const char *filename, int lineno, const char *fmt, ...)
{
	if ( log_flags )
	{
		if ( ( (level==L_INFO) && ((log_flags & LF_NOINFO)==LF_NOINFO) ) ||
		     ( (level==L_DEBUG) && ((log_flags & LF_NODEBUG)==LF_NODEBUG) ) )
		{
			return;
		}
	}
	pthread_mutex_lock(&log_mutex);
	if (!log_flags)
	{
		log_flags=LF_INIT;
#ifdef __DEBUG__
		debug0=getenv("TRACE_DEBUG0");
#endif
		const char *ol=getenv("TRACE_LOG");
		if (ol){
			if (strstr(ol, "noinfo"))
				log_flags|=LF_NOINFO;
			if (strstr(ol, "nodebug"))
				log_flags|=LF_NODEBUG;
			if (strstr(ol, "nocolor"))
				log_flags|=LF_NOCOLOR;
			if (strstr(ol, "syslog")) // Switch to syslog
				trace_log=log_syslog;
		}
		pthread_mutex_unlock(&log_mutex);

		// Call again, now initialized.
		char tmp[1024];
		va_list ap;
		va_start(ap, fmt);
		vsnprintf(tmp,sizeof(tmp),fmt, ap);
		va_end(ap);
		trace_log(level, filename, lineno, tmp);
		return;
	}
	
	filename=basename((char*)filename);
	
#ifdef __DEBUG__
	if ( (level==L_DEBUG0) && (!debug0 || !strstr(debug0, filename)) ){
		pthread_mutex_unlock(&log_mutex);
		return;
	}
#endif

	const char *levelstr[]={ "DEBUG0", "DEBUG", "INFO", "WARNING", "ERROR", "UNKNOWN" };
	const char *levelcolor[]={ "\033[34m", "\033[01;34m", "\033[0m", "\033[01;33m", "\033[31m", "\033[01;31m" };
	
	if ((unsigned int)level>(sizeof(levelstr)/sizeof(levelstr[0]))-1)
		level=(log_level)((sizeof(levelstr)/sizeof(levelstr[0]))-1);
  
  if (!(log_flags&LF_NOCOLOR))
    fprintf(stderr,"%s",levelcolor[level]);

	char datetime[80];
	timeval curTime;
	gettimeofday(&curTime, NULL);
	int milli = curTime.tv_usec / 1000;

	strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", localtime(&curTime.tv_sec));

	char currentTime[84] = "";
	sprintf(currentTime, "%s.%03d", datetime, milli);
	
	char buf[1024];
	string msg = string_format("[%s][%s] ", currentTime, levelstr[level]);
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, 1024, fmt, ap);
	msg += string_format(buf);
	va_end(ap);

	fprintf(stderr, msg.c_str());

	if (!(log_flags&LF_NOCOLOR))
		fprintf(stderr, "\033[0m\n");
	else
		fprintf(stderr, "\n");
			
	pthread_mutex_unlock(&log_mutex);
}


/**
 * @short Performs the log to the syslog
 */
void log_syslog(log_level level, const char *filename, int lineno, const char *fmt, ...)
{
	//#define	LOG_EMERG	0	/* system is unusable */
	//#define	LOG_ALERT	1	/* action must be taken immediately */
	//#define	LOG_CRIT	2	/* critical conditions */
	//#define	LOG_ERR		3	/* error conditions */
	//#define	LOG_WARNING	4	/* warning conditions */
	//#define	LOG_NOTICE	5	/* normal but significant condition */
	//#define	LOG_INFO	6	/* informational */
	//#define	LOG_DEBUG	7	/* debug-level messages */

	char pri[] = { 7, 7, 6, 4, 3 };
	if (level>sizeof(pri))
		return;
	
	va_list ap;
	va_start(ap, fmt);
	vsyslog(pri[level], fmt, ap);
	va_end(ap);
}
