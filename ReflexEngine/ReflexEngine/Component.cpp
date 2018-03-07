#include "Component.h"
#include "Object.h"

namespace Reflex
{
	namespace Components
	{
		Component::Component( Object& object, BaseHandle handle )
			: Entity( handle )
			, m_object( object )
		{

		}

		Reflex::Core::Object& Component::GetObject()
		{
			return m_object;
		}

		const Object& Component::GetObject() const 
		{ 
			return m_object;
		}
	}
}