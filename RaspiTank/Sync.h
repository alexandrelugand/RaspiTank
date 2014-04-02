#pragma once

#include <pthread.h>

typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;

//
// Critical section
//
typedef struct _CRITICAL_SECTION
{
	pthread_mutex_t mutex;

} CRITICAL_SECTION, *LPCRITICAL_SECTION;

enum OBJECT_MAGIC
{
	EVENT_MAGIC,
	NULL_MAGIC
};

typedef struct _OBJECT_HEADER
{
	BYTE magic;
	BYTE count;
	_OBJECT_HEADER(BYTE _magic) : magic(_magic), count(1) {}

} OBJECT_HEADER, *LPOBJECT_HEADER;

typedef struct _EVENT_OBJECT
{
	OBJECT_HEADER header;
	pthread_mutex_t lock;
	pthread_cond_t  cond;
	bool is_set;
	bool is_pulse;
	bool is_manual_reset;

	_EVENT_OBJECT(bool _is_set, bool _is_manual_reset)
		: header(EVENT_MAGIC), is_set(_is_set),
		is_pulse(false), is_manual_reset(_is_manual_reset)
	{
		pthread_cond_init(&cond, NULL);
		pthread_mutex_init(&lock, NULL);
	}
	~_EVENT_OBJECT()
	{
		pthread_cond_destroy(&cond);
		pthread_mutex_destroy(&lock);
	}

} EVENT_OBJECT, *LPEVENT_OBJECT;

void InitializeCriticalSection(LPCRITICAL_SECTION);
void EnterCriticalSection(LPCRITICAL_SECTION);
void LeaveCriticalSection(LPCRITICAL_SECTION);
void DeleteCriticalSection(LPCRITICAL_SECTION);

typedef void * HANDLE;
typedef void VOID;

BOOL CloseHandle(HANDLE hObject);
DWORD GetCurrentThreadId();

//
// Event
//
HANDLE CreateEvent(
	void * skip,
	BOOL bManualReset,
	BOOL bInitialSet,
	const char *);

HANDLE OpenEvent(
	DWORD dwAccess,
	BOOL bInheritHandle,
	const char *);

BOOL SetEvent(HANDLE hObject);

BOOL PulseEvent(HANDLE hObject);

BOOL ResetEvent(HANDLE hObject);

enum WAIT_FOR_CONSTS
{
	INFINITE
};

DWORD WaitForSingleObject(HANDLE hObject, DWORD dwTime);
