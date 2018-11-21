#ifndef AbstractObserver_INCLUDED
#define AbstractObserver_INCLUDED

#include <memory>
#include "Notification.h"

/** The base class for all instantiations of; the Observer and NObserver template classes. **/
namespace loss 
{

class AbstractObserver
{
public:
	AbstractObserver();
	AbstractObserver(const AbstractObserver& observer);
	virtual ~AbstractObserver();
	AbstractObserver& operator = (const AbstractObserver& observer);

	virtual void notify(std::shared_ptr<Notification> pNf) const = 0;
	virtual bool equals(const AbstractObserver& observer) const = 0;
	virtual bool accepts(Notification* pNf) const = 0;
	virtual AbstractObserver* clone() const = 0;
	virtual void disable() = 0;
};

}

#endif // Foundation_AbstractObserver_INCLUDED
