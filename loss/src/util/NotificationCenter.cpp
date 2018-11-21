#include "NotificationCenter.h"
#include "Notification.h"
#include "Observer.h"
#include "LibSingleton.h"

namespace loss 
{

NotificationCenter::NotificationCenter()
{
}


NotificationCenter::~NotificationCenter()
{
}


void NotificationCenter::addObserver(const AbstractObserver& observer)
{
	 std::lock_guard<std::mutex> lock(_mutex);
	_observers.push_back(std::shared_ptr<AbstractObserver>(observer.clone()));
}


void NotificationCenter::removeObserver(const AbstractObserver& observer)
{
	std::lock_guard<std::mutex> lock(_mutex);
	for (ObserverList::iterator it = _observers.begin(); it != _observers.end(); ++it)
	{
		if (observer.equals(**it))
		{
			(*it)->disable();
			_observers.erase(it);
			return;
		}
	}
}


bool NotificationCenter::hasObserver(const AbstractObserver& observer) const
{
	std::lock_guard<std::mutex> lock(_mutex);
	for (ObserverList::const_iterator it = _observers.begin(); it != _observers.end(); ++it)
		if (observer.equals(**it)) return true;

	return false;
}


void NotificationCenter::postNotification(std::shared_ptr<Notification> pNotification)
{
    ObserverList observersToNotify;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        observersToNotify.insert( observersToNotify.end(), _observers.begin(), _observers.end() );
    }

	for (ObserverList::iterator it = observersToNotify.begin(); it != observersToNotify.end(); ++it)
	{
		(*it)->notify(pNotification);
	}
}


bool NotificationCenter::hasObservers() const
{
	std::lock_guard<std::mutex> lock(_mutex);

	return !_observers.empty();
}


std::size_t NotificationCenter::countObservers() const
{
	std::lock_guard<std::mutex>  lock(_mutex);

	return _observers.size();
}


NotificationCenter& NotificationCenter::defaultCenter()
{
    return *( CSingleton<NotificationCenter>::Instance() );
}

}
