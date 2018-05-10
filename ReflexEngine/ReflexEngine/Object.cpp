#include "Object.h"
#include "World.h"
#include "Component.h"
#include "TransformComponent.h"

namespace Reflex
{
	namespace Core
	{
		Object::Object( World& world, BaseHandle objectHandle )
			: Entity( objectHandle )
			, m_world( world )
			, m_cachedTransformType( Type( typeid( Reflex::Components::Transform ) ) )
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

		void Object::RemoveAllComponents()
		{
			std::for_each( m_components.begin(), m_components.end(), [&]( const std::pair< Type, BaseHandle >& component )
			{
				m_world.DestroyComponent( component.first, component.second );
			} );

			m_components.clear();
		}

		BaseHandle Object::GetComponent( Type componentType ) const
		{
			if( componentType == m_cachedTransformType )
				return m_components[0].second;

			for( auto& componentHandle : m_components )
			{
				if( !componentHandle.second.IsValid() )
					continue;

				if( componentType == componentHandle.first )
					return componentHandle.second;
			}

			return BaseHandle();
		}

		BaseHandle Object::GetComponent( const unsigned index ) const
		{
			if( index >= m_components.size() )
				return BaseHandle();

			return m_components[index].second;
		}

		TransformHandle Object::GetTransform() const
		{
			return TransformHandle( m_components[0].second );
		}

		World& Object::GetWorld() const
		{
			return m_world;
		}
	}
}