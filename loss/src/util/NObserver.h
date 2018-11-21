#ifndef NObserver_INCLUDED
#define NObserver_INCLUDED

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
 *  This class template is quite similar to the Observer class
 *  template. The only difference is that the NObserver
 *  expects the callback function to accept a const AutoPtr& 
 *  instead of a plain pointer as argument, thus simplifying memory 
 *  management.                                                     
 *
 * @tparam C
 * @tparam N
 * 
 **/
namespace loss 
{

template <class C, class N>
class NObserver: public AbstractObserver
{
public:
	typedef void (C::*Callback)(const std::shared_ptr<N> );

	NObserver(C& object, Callback method): 
		_pObject(&object), 
		_method(method)
	{
	}
	
	NObserver(const NObserver& observer):
		AbstractObserver(observer),
		_pObject(observer._pObject), 
		_method(observer._method)
	{
	}
	
	~NObserver()
	{
	}
	
	NObserver& operator = (const NObserver& observer)
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
        std::lock_guard<std::mutex> lock(_mutex);

		if (_pObject)
		{
			N* pCastNf = dynamic_cast<N*>(pNf.get());
			if (pCastNf)
			{
				(_pObject->*_method)(ptr);
			}
		}
	}
	
	bool equals(const AbstractObserver& abstractObserver) const
	{
		const NObserver* pObs = dynamic_cast<const NObserver*>(&abstractObserver);
		return pObs && pObs->_pObject == _pObject && pObs->_method == _method;
	}

	bool accepts(Notification* pNf) const
	{
		return dynamic_cast<N*>(pNf) != 0;
	}
	
	AbstractObserver* clone() const
	{
		return new NObserver(*this);
	}
	
	void disable()
	{
		Poco::Mutex::ScopedLock lock(_mutex);
		
		_pObject = 0;
	}

private:
	NObserver();

	C*       _pObject;
	Callback _method;
	mutable Poco::Mutex _mutex;
};

}
#endif // Foundation_NObserver_INCLUDED
