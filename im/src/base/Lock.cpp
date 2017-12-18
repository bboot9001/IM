#include "Lock.h"

BLock::BLock()
{
#ifdef _WIN32
	InitializeCriticalSection(&m_critical_section);
#else
	pthread_mutex_init(&m_lock,NULL);
#endif
}

BLock::~BLock()
{
#ifdef _WIN32
	DeleteCriticalSection(&m_critical_section);
#else
	pthread_mutex_destroy(&m_lock);
#endif
}

void BLock::lock()
{
#ifdef _WIN32
	EnterCriticalSection(&m_critical_section);
#else
	pthread_mutex_lock(&m_lock);
#endif
}

void BLock::unlock()
{
#ifdef _WIN32
	LeaveCriticalSection(&m_critical_section);
#else
	pthread_mutex_unlock(&m_lock);
#endif
}

#ifndef _WIN32
bool BLock::try_lock()
{
    return pthread_mutex_trylock(&m_lock) == 0;
}
#endif

BAutoLock::BAutoLock(BLock* pLock)
{
    m_pLock = pLock;
    if(NULL != m_pLock)
        m_pLock->lock();
}

BAutoLock::~BAutoLock()
{
    if(NULL != m_pLock)
        m_pLock->unlock();
}
