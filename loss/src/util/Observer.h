#ifndef Observer_INCLUDED
#define Observer_INCLUDED

#include <memory>
#include <mutex>
#include "AbstractObserver.h"

/**
 * @brief: 
 *
 *  This template class implements an adapter that sits between
 *  a NotificationCenter and an object receiving notifications
 *  from it. It is quite similar in concept to the 
 *  RunnableAdapter, but provides some NotificationCenter
 *  specific additional methods.
 *  See the NotificationCenter class for information on how
 *  to use this template class.
 *                                                             
 *  Instead of the Observer class template, you might want to
 *  use the NObserver class template, which uses an std::shared_ptr to
 *  pass the Notification to the callback function, thus freeing
 *  you from memory management issues.
 *
 * @tparam C:  class of Object 
 * @tparam N:  calss of notification, Object' method params
 *
 */
namespace loss 
{
template <class C, class N>
class Observer: public AbstractObserver
{
public:
	typedef void (C::*Callback)(N*);

	Observer(std::shared_ptr<C> object, Callback method): 
		_pObject(object), 
		_method(method)
	{
	}
	
	Observer(const Observer& observer)
        : AbstractObserver(observer),
        _pObject(observer._pObject), 
		_method(observer._method)
	{
	}
	
	~Observer()
	{
	}
	
	Observer& operator = (const Observer& observer)
	{
		if (&observer != this)
		{
			_pObject = observer._pObject;
			_method  = observer._method;
		}
		return *this;
	}
	
	void notify(std::shared_ptr<Notification> pNf) const
	{
        std::lock_guard<std::mutex>  lock(_mutex);

		if (_pObject)
		{
			N* pCastNf = dynamic_cast<N*>(pNf.get());
			if (pCastNf)
			{
				(_pObject.get()->*_method)( dynamic_cast<N*>(pNf.get()) );
			}
		}
	}
	
	bool equals(const AbstractObserver& abstractObserver) const
	{
		const Observer* pObs = dynamic_cast<const Observer*>(&abstractObserver);
		return pObs && pObs->_pObject == _pObject && pObs->_method == _method;
	}

	bool accepts(Notification* pNf) const
	{
		return dynamic_cast<N*>(pNf) != 0;
	}
	
	AbstractObserver* clone() const
	{
		return new Observer(*this);
	}
	
	void disable()
	{
        std::lock_guard<std::mutex>  lock(_mutex);
		_pObject = nullptr;
	}
	
private:
    Observer();

    std::shared_ptr<C>                _pObject;
	Callback                          _method;
	mutable std::mutex                _mutex;
};


}

#endif // Foundation_Observer_INCLUDED
