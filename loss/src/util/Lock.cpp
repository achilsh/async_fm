#include "Lock.h"


using loss::Lock;
using loss::ReadLock;
using loss::WriteLock;
using loss::PtrLock;

using loss::LockType;
using loss::Lockable;


static inline void takeLock(Lockable& lockable, LockType lockType) {
   switch ( lockType )
    {
       case loss::VOCAL_READLOCK:
       {
          lockable.readlock();
          break;
       }
	    
       case loss::VOCAL_WRITELOCK:
       {
          lockable.writelock();
          break;
       }
       
       default:
       {
          lockable.lock();
          break;
       }
    }
}

Lock::Lock(Lockable & lockable, LockType lockType)
   : myLockable(lockable)
{
   takeLock(lockable, lockType);
}

Lock::~Lock()
{
    myLockable.unlock();
}

ReadLock::ReadLock(Lockable & lockable)
   : Lock(lockable, VOCAL_READLOCK)
{
}

WriteLock::WriteLock(Lockable & lockable)
   : Lock(lockable, VOCAL_WRITELOCK)
{
}


PtrLock::PtrLock(Lockable* lockable, LockType lockType)
   : myLockable(lockable)
{
   if (lockable)
      takeLock(*lockable, lockType);
}

PtrLock::~PtrLock()
{
    if (myLockable)
        myLockable->unlock();
}
