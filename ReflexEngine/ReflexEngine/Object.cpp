#include "Object.h"
#include "World.h"
#include "Component.h"
#include "TransformComponent.h"

namespace Reflex
{
	namespace Core
	{
		Object::Object( World& world )
			: m_world( world )
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

		void Object::RemoveComponentInternal(const Core::Type& componentType, const BaseHandle& handle)
		{
			const auto found = std::find_if(m_components.begin(), m_components.end(), [&](const std::pair< Type, BaseHandle >& component)
				{
					return component.second == handle;
				});

			if (found != m_components.end())
			{
				m_world.DestroyComponent(componentType, found->second);
				m_components.erase(found);
			}
		}

		void Object::RemoveComponentInternal(const Core::Type& componentType)
		{
			const auto found = std::find_if(m_components.begin(), m_components.end(), [&componentType](const std::pair< Type, BaseHandle >& componentHandle)
				{
					if (!componentHandle.second.IsValid())
						return false;

					return componentType == componentHandle.first;
				});

			if (found != m_components.end())
			{
				m_world.DestroyComponent(componentType, found->second);
				m_components.erase(found);
			}
		}

		void Object::RemoveComponentsInternal(const Core::Type& componentType)
		{
			m_components.erase(std::remove_if(m_components.begin(), m_components.end(), [&](const std::pair< Type, BaseHandle >& component)
				{
					if (!component.second.IsValid())
						return false;

					if (componentType != component.first)
						return false;

					m_world.DestroyComponent(componentType, component.second);

					return true;
				}
			), m_components.end());
		}
	}
}