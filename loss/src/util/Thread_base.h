/**
 * @file: ThreadIf.h
 * @brief:  定义一个线程基类
 *
 * @author:  wusheng Hu
 * @version: v0x00001
 * @date: 2018-10-14
 */

#if !defined(Thread_base_h_)
#define Thread_base_h_

#include <pthread.h>
#include <mutex>
#include <condition_variable>

namespace util 
{

/** 
   @brief A wrapper class to create and spawn a thread.  

   It is a base class.
   ThreadIf::thread() is a pure virtual method .

   To use this class, derive from it and override the thread() method.
   To start the thread, call the run() method.  The code in thread() will
   run in a separate thread.

   Call shutdown() from the constructing thread to shut down the
   code.  This will set the bool shutdown_ to true.  The code in
   thread() should react properly to shutdown_ being set, by
   returning.  Call join() to join the code.

   Sample:
   @code
   DerivedThreadIf thread;
   thread.run();
   // ... do stuff ...
   thread.shutdown();
   thread.join();
   @endcode
*/
class ThreadIf
{
   public:
      ThreadIf();
      virtual ~ThreadIf();

      // runs the code in thread() .  Returns immediately
      virtual void run();

      // joins to the thread running thread()
      void join();

      // guarantees resources consumed by thread are released when thread terminates
      // after this join can no-longer be used
      void detach();

      // request the thread running thread() to return, by setting  mShutdown
      virtual void shutdown();

      //waits for waitMs, or stops waiting and returns true if shutdown was
      //called
      virtual bool waitForShutdown(int ms) const;

      // returns true if the thread has been asked to shutdown or not running
      bool isShutdown() const;

      typedef pthread_t Id;
      static Id selfId();

      typedef pthread_key_t TlsKey;
      typedef void TlsDestructor(void*);

      /** This function follows pthread_key_create() signature */
      static int tlsKeyCreate(TlsKey &key, TlsDestructor *destructor);
      /** This function follows pthread_key_delete() signature */
      static int tlsKeyDelete(TlsKey key);
      /** This function follows pthread_setspecific() signature */
      static int tlsSetValue(TlsKey key, const void *val);
      /** This function follows pthread_getspecific() signature */
      static void *tlsGetValue(TlsKey key);


      /* thread is a virtual method.  Users should derive and define
        thread() such that it returns when isShutdown() is true.
      */
      virtual void thread() = 0;

   protected:
     Id                                 mId;

      bool                              mShutdown;
      mutable std::mutex                mShutdownMutex;
      mutable std::condition_variable   mShutdownCondition;

   private:
      // Suppress copying
      ThreadIf(const ThreadIf &);
      const ThreadIf & operator=(const ThreadIf &);
};
}

#endif
