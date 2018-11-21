#ifndef NotificationCenter_INCLUDED
#define NotificationCenter_INCLUDED


#include "Notification.h"
#include <mutex>
#include <memory>
#include <vector>
#include <cstddef>

namespace loss 
{

class AbstractObserver;

class NotificationCenter
{
public:
	NotificationCenter();

	~NotificationCenter();

    /**
     * @brief: addObserver 
     *          Registers an observer with the NotificationCenter.
     * @param observer
     *
     *  Usage:
     *      Observer<MyClass, MyNotification> obs(*this, &MyClass::handleNotification);
     *      notificationCenter.addObserver(obs); 
     *      MyClass is target obj.  MyNotification is notification
     *      Observer param: 1st is target obj; 2nd is target notify_callback
     *                                                                                
     *      Alternatively, the NObserver template class can be used instead of Observer. 
     ***/
	void addObserver(const AbstractObserver& observer);

    /**
     * @brief: removeObserver 
     *
	 *	 Unregisters an observer with the NotificationCenter.
     * @param observer
     */
	void removeObserver(const AbstractObserver& observer);

    /**
     * @brief: hasObserver 
     *
     * @param observer
     *  
     * @return 
     *          true if the observer is registered with this NotificationCenter.
     */
	bool hasObserver(const AbstractObserver& observer) const;

    /**
     * @brief: postNotification 
     *
     *  Posts a notification to the NotificationCenter.
     *  The NotificationCenter then delivers the notification
     *  to all interested observers.
     *  If an observer throws an exception, dispatching terminates
     *  and the exception is rethrown to the caller.
     *  Ownership of the notification object is claimed and the
     *  notification is released before returning. Therefore,
     *  a call like notificationCenter.postNotification(new MyNotification);
     *
     * @param pNotification
     */
	void postNotification(std::shared_ptr<Notification> pNotification);

    /**
     * @brief: hasObservers 
     *
     * @return 
     *   true if there is at least one registered observer.
     *   Can be used to improve performance if an expensive notification
     *   shall only be created and posted if there are any observers.
     */
	bool hasObservers() const;
		
    /**
     * @brief: countObservers 
     *
     * @return: the number of registered observers. 
     */
	std::size_t countObservers() const;

    /**
     * @brief: defaultCenter 
     *
     * @return: a reference to the default 
     */
	static NotificationCenter& defaultCenter();

private:
	typedef std::shared_ptr<AbstractObserver> AbstractObserverPtr;
	typedef std::vector<AbstractObserverPtr> ObserverList;

	ObserverList        _observers;
	mutable std::mutex  _mutex;
};


}
#endif // Foundation_NotificationCenter_INCLUDED
