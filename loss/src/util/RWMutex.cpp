#include "RWMutex.h"
#include "Lock.h"
#include "ResipAssert.h"

using loss::RWMutex;
using loss::Lock;

RWMutex::RWMutex()
   :   Lockable(),
       mReaderCount(0),
       mWriterHasLock(false),
       mPendingWriterCount(0)
{
}


RWMutex::~RWMutex()
{
}


void
RWMutex::readlock()
{
   Lock    lock(mMutex);

   while ( mWriterHasLock || mPendingWriterCount > 0 )
   {
      mReadCondition.wait(mMutex);
   }

   mReaderCount++;
}


void
RWMutex::writelock()
{
   Lock    lock(mMutex);

   mPendingWriterCount++;

   while ( mWriterHasLock || mReaderCount > 0 )
   {
      mPendingWriteCondition.wait(mMutex);
   }

   mPendingWriterCount--;

   mWriterHasLock = true;
}


void
RWMutex::lock()
{
   writelock();
}


void
RWMutex::unlock()
{
   Lock    lock(mMutex);

   // Unlocking a write lock.
   //
   if ( mWriterHasLock )
   {
      resip_assert( mReaderCount == 0 );

      mWriterHasLock = false;

      // Pending writers have priority. Could potentially starve readers.
      //
      if ( mPendingWriterCount > 0 )
      {
         mPendingWriteCondition.signal();
      }

      // No writer, no pending writers, so all the readers can go.
      //
      else
      {
         mReadCondition.broadcast();
      }

   }

   // Unlocking a read lock.
   //
   else
   {
      resip_assert( mReaderCount > 0 );

      mReaderCount--;

      if ( mReaderCount == 0 && mPendingWriterCount > 0 )
      {
         mPendingWriteCondition.signal();
      }
   }
}

unsigned int
RWMutex::readerCount() const
{
   return ( mReaderCount );
}

unsigned int
RWMutex::pendingWriterCount() const
{
   return ( mPendingWriterCount );
}
