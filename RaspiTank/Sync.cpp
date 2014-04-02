#include "Sync.h"

#define TRUE    1
#define FALSE   0

void InitializeCriticalSection(LPCRITICAL_SECTION lpcs)
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

	pthread_mutex_init(&lpcs->mutex, &attr);

	pthread_mutexattr_destroy(&attr);
}

void EnterCriticalSection(LPCRITICAL_SECTION lpcs)
{
	pthread_mutex_lock(&lpcs->mutex);
}

void LeaveCriticalSection(LPCRITICAL_SECTION lpcs)
{
	pthread_mutex_unlock(&lpcs->mutex);
}

void DeleteCriticalSection(LPCRITICAL_SECTION lpcs)
{
	pthread_mutex_destroy(&lpcs->mutex);
}

BOOL CloseHandle(HANDLE hObject)
{
	LPOBJECT_HEADER lpHeader = (LPOBJECT_HEADER)hObject;

	if (EVENT_MAGIC == lpHeader->magic)
	{
		if (0 == --lpHeader->count)
			delete (LPEVENT_OBJECT)hObject;

		return TRUE;
	}

	return FALSE;
}

//
// Event
//
HANDLE CreateEvent(
	void * ignore,  // ignored
	BOOL bManualReset,
	BOOL bInitialSet,
	const char *    // ignored
	)
{
	return (HANDLE) new EVENT_OBJECT(bInitialSet, bManualReset);
}

HANDLE OpenEvent(
	DWORD dwAccess,
	BOOL bInheritHandle,
	const char *)
{
	// nope
	return NULL;
}

BOOL SetEvent(HANDLE hObject)
{
	LPOBJECT_HEADER lpHeader = (LPOBJECT_HEADER)hObject;
	if (EVENT_MAGIC != lpHeader->magic)
		return FALSE;

	LPEVENT_OBJECT lpObject = (LPEVENT_OBJECT)hObject;
	pthread_mutex_lock(&lpObject->lock);
	lpObject->is_set = true;
	lpObject->is_pulse = false;
	pthread_cond_broadcast(&lpObject->cond);
	pthread_mutex_unlock(&lpObject->lock);

	return TRUE;
}

BOOL PulseEvent(HANDLE hObject)
{
	LPOBJECT_HEADER lpHeader = (LPOBJECT_HEADER)hObject;
	if (EVENT_MAGIC != lpHeader->magic)
		return FALSE;

	LPEVENT_OBJECT lpObject = (LPEVENT_OBJECT)hObject;
	pthread_mutex_lock(&lpObject->lock);
	lpObject->is_set = true;
	lpObject->is_pulse = true;
	pthread_cond_signal(&lpObject->cond);
	pthread_mutex_unlock(&lpObject->lock);

	return TRUE;
}

BOOL ResetEvent(HANDLE hObject)
{
	LPOBJECT_HEADER lpHeader = (LPOBJECT_HEADER)hObject;
	if (EVENT_MAGIC != lpHeader->magic)
		return FALSE;

	LPEVENT_OBJECT lpObject = (LPEVENT_OBJECT)hObject;
	pthread_mutex_lock(&lpObject->lock);
	lpObject->is_set = false;
	pthread_mutex_unlock(&lpObject->lock);

	return TRUE;
}

void ms2ts(struct timespec *ts, unsigned long ms)
{
	ts->tv_sec = ms / 1000;
	ts->tv_nsec = (ms % 1000) * 1000000;
}

DWORD WaitForSingleObject(
	HANDLE hObject,
	DWORD dwTime
	)
{
	LPOBJECT_HEADER lpHeader = (LPOBJECT_HEADER)hObject;
	if (EVENT_MAGIC != lpHeader->magic)
		return 0;

	LPEVENT_OBJECT lpObject = (LPEVENT_OBJECT)hObject;
	if (false == lpObject->is_manual_reset)
		lpObject->is_set = false;

	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ms2ts(&ts, dwTime);

	pthread_mutex_lock(&lpObject->lock);
	if (true != lpObject->is_set)
		pthread_cond_timedwait(&lpObject->cond, &lpObject->lock, &ts);
	pthread_mutex_unlock(&lpObject->lock);

	return 1;
}

DWORD GetCurrentThreadId()
{
	return (DWORD)pthread_self();
}