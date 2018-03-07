#pragma once

#include "Common.h"
#include "Component.h"
#include "Entity.h"

namespace Reflex
{
	namespace Core
	{
		class World;

		typedef Handle< class Object > ObjectHandle;

		class Object : public Entity
		{
		public:
			Object( World& world, BaseHandle handle );
			virtual ~Object() { }

			void Destroy();

			// Creates and adds a new component of the template type and returns a handle to it
			template< class T, typename... Args >
			Handle< T > AddComponent( Args&&... args );

			// Removes all components matching the template type
			template< class T >
			void RemoveComponents();

			// Removes first component matching the template type
			template< class T >
			void RemoveComponent();

			// Removes component by handle
			template< class T >
			void RemoveComponent( Handle< T > handle );

			// Checks if this object has a component of template type
			template< class T >
			bool HasComponent() const;

			// Returns a component handle of template type if this object has one
			template< class T >
			Handle< T > GetComponent() const;

			BaseHandle Object::GetComponent( Type componentType ) const;

			World& GetWorld() const;

		private:
			Object() = delete;

		protected:
			World& m_world;
			bool m_destroyed = false;
			std::vector< std::pair< Type, BaseHandle > > m_components;
		};

		// Template definitions
		template< class T, typename... Args >
		Handle< T > Object::AddComponent( Args&&... args )
		{
			auto component = m_world.CreateComponent< T >( ObjectHandle( m_self ), std::forward< Args >( args )... );
			m_components.emplace_back( Type( typeid( T ) ), std::move( component ) );
			return component;
		}

		template< class T >
		void Object::RemoveComponents()
		{
			const auto componentType = Type( typeid( T ) );

			m_components.erase( std::remove_if( m_components.begin(), m_components.end(), [&componentType]( const std::pair< Type, BaseHandle >& component )
			{ 
				if( !component.second.IsValid() )
					return false;

				if( componentType != component.first )
					return false;
				
				m_world.DestroyComponent( component.second );

				return true;
			} 
			), m_components.end() );
		}

		template< class T >
		void Object::RemoveComponent( Handle< T > handle )
		{
			const auto found = std::find_if( m_components.begin(), m_components.end(), [&componentType]( const std::pair< Type, BaseHandle >& component )
			{
				return component.second == handle;
			} );

			if( found != m_components.end() )
			{
				m_world.DestroyComponent( *found );
				m_components.erase( found );
			}
		}

		template< class T >
		void Object::RemoveComponent()
		{
			const auto componentType = Type( typeid( T ) );

			const auto found = std::find_if( m_components.begin(), m_components.end(), [&componentType]( const std::pair< Type, BaseHandle >& componentHandle )
			{
				if( !componentHandle.second.IsValid() )
					return false;

				return componentType == componentHandle.first;
			} );

			if( found != m_components.end() )
			{
				m_world.DestroyComponent( *found );
				m_components.erase( found );
			}
		}

		template< class T >
		bool Object::HasComponent() const
		{
			return GetComponentOfType< T >().IsValid();
		}

		template< class T >
		Handle< T > Object::GetComponent() const
		{
			const auto componentType = Type( typeid( T ) );

			for( auto& componentHandle : m_components )
			{
				if( !componentHandle.second.IsValid() )
					continue;

				if( componentType == componentHandle.first )
					return Handle< T >( componentHandle.second );
			}

			return Handle< T >();
		}
	}
}