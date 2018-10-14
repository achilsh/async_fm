#include "Thread_base.h"
#include <chrono>
#include <iostream>
#include <assert.h>

using namespace util;

extern "C"
{
static void*
threadIfThreadWrapper( void* threadParm )
{
   assert( threadParm );
   ThreadIf* t = static_cast < ThreadIf* > ( threadParm );

   assert( t );
   t->thread();
   return 0;
}
}


ThreadIf::ThreadIf() : 
   mId(0), mShutdown(false), mShutdownMutex()
{
}


ThreadIf::~ThreadIf()
{
   shutdown();
   join();
}

void
ThreadIf::run()
{
   assert(mId == 0);
   // spawn the thread
   if ( int retval = pthread_create( &mId, 0, threadIfThreadWrapper, this) )
   {
      std::cerr << "Failed to spawn thread: " << retval << std::endl;
      assert(0);
      // TODO - ADD LOGING HERE
   }
}

void
ThreadIf::join()
{
   // !kh!
   // perhaps assert instead of returning when join()ed already?
   // programming error?
   //assert(mId == 0);

   if (mId == 0)
   {
      return;
   }
   void* stat;
   if (mId != pthread_self())
   {
      int r = pthread_join( mId , &stat );
      if ( r != 0 )
      {
         //WarningLog( << "Internal error: pthread_join() returned " << r );
         assert(0);
         // TODO
      }
   }
   mId = 0;
}

void
ThreadIf::detach()
{
   pthread_detach(mId);
   mId = 0;
}

ThreadIf::Id
ThreadIf::selfId()
{
   return pthread_self();
}

int
ThreadIf::tlsKeyCreate(TlsKey &key, TlsDestructor *destructor)
{
   return pthread_key_create(&key, destructor);
}

int
ThreadIf::tlsKeyDelete(TlsKey key)
{
   return pthread_key_delete(key);
}

int
ThreadIf::tlsSetValue(TlsKey key, const void *val)
{
   return pthread_setspecific(key, val);
}

void *
ThreadIf::tlsGetValue(TlsKey key)
{
   return pthread_getspecific(key);
}


void
ThreadIf::shutdown()
{
    std::unique_lock<std::mutex> lock(mShutdownMutex);
   if (!mShutdown)
   {
      mShutdown = true;
      mShutdownCondition.notify_one();
   }
}

bool
ThreadIf::waitForShutdown(int ms) const
{
    std::unique_lock<std::mutex> lock(mShutdownMutex);
   if(!mShutdown)
   {
      mShutdownCondition.wait_for(lock, std::chrono::milliseconds(ms));
   }
   return mShutdown;
}

bool
ThreadIf::isShutdown() const
{
   std::lock_guard<std::mutex> lock(mShutdownMutex);
   return ( mShutdown );
}
