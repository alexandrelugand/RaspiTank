#pragma once

#ifdef __DEBUG__
#define LDEBUG(...) trace_log(L_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LDEBUG0(...) trace_log(L_DEBUG0, __FILE__, __LINE__, __VA_ARGS__)
#else
#define LDEBUG(...)
#define LDEBUG0(...)
#endif

#define INFO(...) trace_log(L_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define WARNING(...) trace_log(L_WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define ERROR(...) trace_log(L_ERROR, __FILE__, __LINE__, __VA_ARGS__)

enum log_level_e{
	L_DEBUG0=0,
	L_DEBUG=1,
	L_INFO=2,
	L_WARNING=3,
	L_ERROR=4,
};

enum log_flags_e{
	LF_INIT=1,
	LF_NOCOLOR=2,
	LF_SYSLOGINIT=8,
	LF_NOINFO=16,
	LF_NODEBUG=32,
};


typedef enum log_level_e log_level;
extern int log_flags; // For some speedups, as not getting client info if not going to save it.

/// This function can be overwritten with whatever trace_log facility you want to use, same signature
extern void (*trace_log)(log_level level, const char *filename, int lineno, const char *fmt, ...);

void log_stderr(log_level level, const char *filename, int lineno, const char *fmt, ...);
void log_syslog(log_level level, const char *filename, int lineno, const char *fmt, ...);
