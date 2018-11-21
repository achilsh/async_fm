#ifndef Notification_INCLUDED
#define Notification_INCLUDED

#include <mutex> 

/**
 * @brief: 
 *
 *  The base class for all notification classes used
 *  with the NotificationCenter and the NotificationQueue classes.
 *  The Notification class can be used with the AutoPtr template class.
 *  
 */
namespace loss
{

class  Notification
{
public:
	Notification();
    virtual ~Notification();
	virtual std::string name() const;
};


}

#endif // Foundation_Notification_INCLUDED
