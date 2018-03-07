#include "Component.h"
#include "Object.h"

namespace Reflex
{
	namespace Components
	{
		Component::Component( ObjectHandle object, BaseHandle handle )
			: Entity( handle )
			, m_object( object )
		{

		}

		ObjectHandle Component::GetObject() const
		{
			return m_object;
		}
	}
}