#if !defined(RESIP_LOCKABLE_HXX)
#define RESIP_LOCKABLE_HXX 

/** Infrastructure common to VOCAL.<br><br>
 */
namespace loss 
{

/**
   @brief Abstract base-class for Mutexes.
*/
class Lockable
{
   protected:
      Lockable() {};
	
   public:
      virtual ~Lockable() {};
      virtual void lock() = 0;
      virtual void unlock() = 0;
      virtual void readlock() { lock(); }
      virtual void writelock() { lock() ; }
};

}

#endif
