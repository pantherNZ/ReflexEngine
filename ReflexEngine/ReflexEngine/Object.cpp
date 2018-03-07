#include "Object.h"
#include "World.h"
#include "Component.h"

namespace Reflex
{
	namespace Core
	{
		Object::Object( World& world, BaseHandle handle )
			: Entity( handle )
			, m_world( world )
		{

		}

		void Object::Destroy()
		{
			if( !m_destroyed )
			{
				m_world.DestroyObject( m_self );
				m_destroyed = true;
			}
		}

		BaseHandle Object::GetComponentOfType( Type componentType ) const
		{
			for( auto& componentHandle : m_components )
			{
				if( !componentHandle.second.IsValid() )
					continue;

				if( componentType == componentHandle.first )
					return componentHandle.second;
			}

			return BaseHandle();
		}

		World& Object::GetWorld() const
		{
			return m_world;
		}
	}
}