#include "Object.h"
#include "World.h"
#include "Component.h"

namespace Reflex
{
	namespace Core
	{
		Object::Object( World& world, ObjectHandle handle )
			: m_world( world )
			, m_self( handle )
		{

		}

		void Object::Destroy()
		{
			if( !m_destroyed && !m_active )
			{
				m_world.DestroyObject( m_self );
				m_destroyed = true;
			}
		}

		void Object::RemoveComponent( ComponentHandle handle )
		{
			m_components.erase( std::find_if( m_components.begin(), m_components.end(), [&]( const ComponentHandle& componentHandle )
			{
				if( handle == componentHandle )
					m_world.DestroyComponent( handle );
				return handle == componentHandle;
			} ) );
		}

		void Object::draw( sf::RenderTarget& target, sf::RenderStates states ) const
		{
			states.transform *= getTransform();

			Draw( target, states );
		}
	}
}