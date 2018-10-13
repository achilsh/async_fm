#if !defined(RESIP_CONDITION_HXX)
#define RESIP_CONDITION_HXX

#if defined(WIN32)
#  include <windows.h>
#  include <winbase.h>
#else
#  include <pthread.h>
#endif

// !kh!
// Attempt to resolve POSIX behaviour conformance for win32 build.
#define RESIP_CONDITION_WIN32_CONFORMANCE_TO_POSIX

namespace loss 
{

class Mutex;

/**
  @brief 
  A <a href="http://en.wikipedia.org/wiki/Condition_variable#Condition_variables"> 
  condition variable</a> that can be signaled or waited on, wraps POSIX/Windows 
  implementations depending on environment.

   Here's an example (from ThreadIf):

@code
  void
  ThreadIf::shutdown()
  {
     Lock lock(mShutdownMutex);
     if (!mShutdown)
     {
        mShutdown = true;
        mShutdownCondition.signal();
     }
  }

  bool
  ThreadIf::waitForShutdown(int ms) const
  {
     Lock lock(mShutdownMutex);
     mShutdownCondition.wait(mShutdownMutex, ms);
     return mShutdown;
  }
  @endcode

  @see Mutex
*/
class Condition
{
   public:
      Condition();
      virtual ~Condition();

      /** wait for the condition to be signaled
       @param mtx	The mutex associated with the condition variable
      */
      void wait (Mutex& mtx);
      /** wait for the condition to be signaled
       @param mtx   The mutex associated with the condition variable
       @retval true The condition was woken up by activity
       @retval false Timeout or interrupt.
      */
      bool wait (Mutex& mutex, unsigned int ms);

      // !kh!
      //  deprecate these?
      void wait (Mutex* mutex);
      bool wait (Mutex* mutex, unsigned int ms);

      /** Signal one waiting thread.
       @return 0 Success
       @return errorcode The error code of the failure
       */
      void signal();

      /** Signal all waiting threads.
       @return 0 Success
       @return errorcode The error code of the failure
       */
      void broadcast();

   private:
      // !kh!
      //  no value sematics, therefore private and not implemented.
      Condition (const Condition&);
      Condition& operator= (const Condition&);

   private:
#ifdef WIN32
#  ifdef RESIP_CONDITION_WIN32_CONFORMANCE_TO_POSIX
   // !kh!
   // boost clone with modification (license text below)
   void enterWait ();
   void* m_gate;
   void* m_queue;
   void* m_mutex;
   unsigned m_gone;  // # threads that timed out and never made it to m_queue
   unsigned long m_blocked; // # threads blocked on the condition
   unsigned m_waiting; // # threads no longer waiting for the condition but
                        // still waiting to be removed from m_queue
#  else
   HANDLE mId;
#  endif
#else
   mutable  pthread_cond_t mId;
#endif
};

}

#endif
