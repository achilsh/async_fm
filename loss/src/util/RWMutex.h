#if !defined(RESIP_RWMUTEX_HXX)
#define RESIP_RWMUTEX_HXX 

static const char* const resipRWMutex_hxx_Version =
   "$Id: RWMutex.hxx,v 1.3 2003/06/02 20:52:32 ryker Exp $";

#include "Lockable.h"
#include "Mutex.h"
#include "Condition.h"

namespace loss 
{

/**
   @brief Wraps the readers/writers mutex implementation on your platform.

   @note A readers/writers mutex is a mutex that can be locked in two differing
      ways:
         - A read-lock: Prevents other threads from obtaining a write-lock, but
            not other read-locks.
         - A write-lock: Prevents other threads from obtaining either a 
            read-lock or write-lock.

      Usually, if a thread attempts to aquire a write-lock while read-locks are
      being held, this will prevent any further read-locks from being obtained,
      until the write-lock is aquired and released. This prevents the "writer
      starvation" problem.
*/
class RWMutex : public Lockable
{
    public:
      RWMutex();
      ~RWMutex();
      void readlock();
      void writelock();
      void lock();
      void unlock();
      unsigned int readerCount() const;
      unsigned int pendingWriterCount() const;
      
   private:
      Mutex mMutex;
      Condition mReadCondition;
      Condition mPendingWriteCondition;
      unsigned int mReaderCount;
      bool mWriterHasLock;
      unsigned int mPendingWriterCount;
};

}

#endif 
