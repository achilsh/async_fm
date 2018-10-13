/*
namespace std
{
typedef wchar_t wint_t;
typedef unsigned int size_t;
}

*/
#include "ResipAssert.h"
#include <cerrno>
#include "Mutex.h"

#if defined(WIN32)
#  include <windows.h>
#  include <winbase.h>
#else
#  include <pthread.h>
#endif

using namespace loss;

Mutex::Mutex()
{
#ifndef WIN32
    int  rc = pthread_mutex_init(&mId,0);
    (void)rc;
    resip_assert( rc == 0 );
#else
	// Note:  Windows Critical sections are recursive in nature and perhaps
	//        this implementation calls for a non-recursive implementation
	//        (since there also exists a RecursiveMutex class).  The effort
	//        to make this non-recursive just doesn't make sense though. (SLG)
	InitializeCriticalSection(&mId);
#endif
}


Mutex::~Mutex ()
{
#ifndef WIN32
    int  rc = pthread_mutex_destroy(&mId);
    (void)rc;
    resip_assert( rc != EBUSY );  // currently locked 
    resip_assert( rc == 0 );
#else
	DeleteCriticalSection(&mId);
#endif
}


void
Mutex::lock()
{
#ifndef WIN32
    int  rc = pthread_mutex_lock(&mId);
    (void)rc;
    resip_assert( rc != EINVAL );
    resip_assert( rc != EDEADLK );
    resip_assert( rc == 0 );
#else
	EnterCriticalSection(&mId);
#endif
}

void
Mutex::unlock()
{
#ifndef WIN32
    int  rc = pthread_mutex_unlock(&mId);
    (void)rc;
    resip_assert( rc != EINVAL );
    resip_assert( rc != EPERM );
    resip_assert( rc == 0 );
#else
	LeaveCriticalSection(&mId);
#endif
}

#ifndef WIN32
pthread_mutex_t*
Mutex::getId() const
{
    return ( &mId );
}
#endif
