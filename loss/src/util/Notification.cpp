#include "Notification.h"
#include <typeinfo>


namespace loss
{

Notification::Notification()
{
}


Notification::~Notification()
{
}


std::string Notification::name() const
{
	return typeid(*this).name();
}

}
