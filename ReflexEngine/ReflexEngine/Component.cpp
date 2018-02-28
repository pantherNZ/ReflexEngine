#include "Component.h"
#include "Object.h"

namespace Reflex
{
	namespace Components
	{
		Component::Component( Object& object )
			: mObject( object ) 
		{

		}

		Object& Component::GetObject() 
		{ 
			return mObject; 
		}

		const Object& Component::GetObject() const 
		{ 
			return mObject; 
		}
	}
}