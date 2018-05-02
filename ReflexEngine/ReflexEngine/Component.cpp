#include "Component.h"
#include "Object.h"

namespace Reflex
{
	namespace Components
	{
		Component::Component( const ObjectHandle& object, const BaseHandle& handle )
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