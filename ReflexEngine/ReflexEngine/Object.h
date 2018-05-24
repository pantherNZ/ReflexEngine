#pragma once

#include "Precompiled.h"
#include "Component.h"
#include "Entity.h"
#include "TransformComponent.h"

namespace Reflex
{
	namespace Core
	{
		class World;

		class Object : public Entity, private sf::NonCopyable
		{
		public:
			Object( World& world );
			virtual ~Object() { }

			void Destroy();

			// Creates and adds a new component of the template type and returns a handle to it
			template< class T, typename... Args >
			Handle< T > AddComponent( Args&&... args );

			// Removes all components
			void RemoveAllComponents();

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

			// Returns a component handle of template type if this object has one (index is the nth number of the component looking for)
			template< class T >
			Handle< T > GetComponent( const unsigned index = 0U ) const;

			// Returns a component handle of the template type at the specified index in the component array
			template< class T >
			Handle< T > GetComponentAt( const unsigned index ) const;

			BaseHandle GetComponent( Type componentType ) const;
			BaseHandle GetComponent( const unsigned index ) const;

			template< class T >
			std::vector< Handle< T > > GetComponents() const;

			// Copy components from another object to this object
			template< typename T, typename... Args >
			void CopyComponentsFrom( const ObjectHandle& other );

			TransformHandle GetTransform() const;

			World& GetWorld() const;

		protected:
			Object() = delete;

			// Use SFINAE to remove the base case where there is 0 argument types (above function calls itself recursively until we reach 0 template arguments)
			template< typename... Args >
			typename std::enable_if< sizeof...( Args ) == 0 >::type CopyComponentsFrom( const ObjectHandle& other ) {}

		protected:
			World& m_world;
			bool m_destroyed = false;
			std::vector< std::pair< Type, BaseHandle > > m_components;

		private:
			Type m_cachedTransformType;
		};

		// Template definitions
		template< class T, typename... Args >
		Handle< T > Object::AddComponent( Args&&... args )
		{
			auto component = m_world.CreateComponent< T >( ObjectHandle( m_self ), std::forward< Args >( args )... );
			m_components.emplace_back( Type( typeid( T ) ), component );
			component->OnConstructionComplete();
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
				m_world.DestroyComponent( found->second );
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
				m_world.DestroyComponent( componentType, found->second );
				m_components.erase( found );
			}
		}

		template< class T >
		bool Object::HasComponent() const
		{
			return GetComponent< T >().IsValid();
		}

		template< class T >
		Handle< T > Object::GetComponent( const unsigned index /*= 0U*/ ) const
		{
			const auto componentType = Type( typeid( T ) );
			unsigned count = index;

			if( componentType == m_cachedTransformType )
				return m_components[0].second;

			for( auto& componentHandle : m_components )
			{
				if( !componentHandle.second.IsValid() )
					continue;

				if( componentType == componentHandle.first && !( count-- ) )
					return Handle< T >( componentHandle.second );
			}

			return Handle< T >();
		}

		template< class T >
		Handle< T > Object::GetComponentAt( const unsigned index ) const
		{
			if( index >= m_components.size() )
				return Handle< T >();

			return Handle< T >( m_components[index].second );
		}

		template< class T >
		std::vector< Handle< T > > Object::GetComponents() const
		{
			const auto componentType = Type( typeid( T ) );

			if( componentType == m_cachedTransformType )
				return { m_components[0].second };

			std::vector< Handle< T > > results;

			for( auto& componentHandle : m_components )
			{
				if( !componentHandle.second.IsValid() )
					continue;

				if( componentType == componentHandle.first )
					results.push_back( Handle< T >( componentHandle.second ) );
			}

			return std::move( results );
		}

		template< typename T, typename... Args >
		void Object::CopyComponentsFrom( const ObjectHandle& other )
		{
			auto component = other->GetComponent< T >();

			if( !component )
				return;

			if( GetComponent< T >().IsValid() )
				RemoveComponent< T >();

			AddComponent< T >( *component.Get() );

			CopyComponentsFrom< Args... >( other );
		}
	}
}