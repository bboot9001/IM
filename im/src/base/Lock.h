/*================================================================
*   Copyright (C) 2017 All rights reserved.
*   
*   文件名称：Lock.h
*   创 建 者：bboot9001
*   邮    箱：wwwang9001@163.com
*   创建日期：2017年12月18日
*   描    述：
*
================================================================*/
#ifndef __LOCK_H_
#define __LOCK_H_
#ifndef _WIN32
	#include <windows.h>
#else 
	#include "ostype.h"
#endif


class BLock
{
public:
	BLock();
	virtual ~BLock();

public:
	void lock();
	void unlock();

#ifndef _WIN32
	virtual bool try_lock();
	pthread_mutex& getMutex()
	{
		return m_lock;
	}
#endif

private:
#ifndef _WIN32
	CRITICAL_SECTION m_critical_section;
#else 
	pthread_mutex_t m_lock;
#endif

};

class BAutoLock
{
public:
    BAutoLock(BLock* pLock);
    virtual ~BAutoLock();
private:
    BLock* m_pLock;
};

#endif