#pragma once

#include "Common.h"
#include "Handle.h"

namespace Reflex
{
	namespace Core
	{
		class World;

		typedef Handle< class Component > ComponentHandle;
		typedef Handle< class Object > ObjectHandle;

		class Object : public sf::Transformable, public sf::Drawable, private sf::NonCopyable
		{
		public:
			Object( World& world, ObjectHandle handle );

			void Destroy();

			// Creates and adds a new component of the template type and returns a handle to it
			template< class T >
			ComponentHandle AddComponent();

			// Removes all components matching the template type
			template< class T >
			void RemoveAllComponentsOfType();

			// Removes first components matching the template type
			template< class T >
			void RemoveFirstComponentOfType();

			// Removes 
			void RemoveComponent( ComponentHandle handle );

		protected:
			virtual void Draw( sf::RenderTarget& target, sf::RenderStates states ) const { }

		private:
			void draw( sf::RenderTarget& target, sf::RenderStates states ) const final;

			Object() = delete;

		public:
			bool m_active = false;
			ObjectHandle m_self;

		protected:
			World& m_world;
			bool m_destroyed = false;
			std::vector< ComponentHandle > m_components;
		};

		// Template definitions
		template< class T >
		ComponentHandle Object::AddComponent()
		{
			auto component = m_world.CreateComponent< T >();
			m_components.push_back( std::move( component ) );
			return component;
		}

		template< class T >
		void Object::RemoveAllComponentsOfType()
		{
			const auto componentType = ComponentType( typeid( T ) );

			m_components.erase( std::remove_if( m_components.begin(), m_components.end(), [&componentType]( const ComponentHandle& componentHandle )
			{ 
				if( !componentHandle.Get() )
					return false;

				return componentType == ComponentType( typeid( *componentHandle.Get() ) )
			} 
			), m_components.end() );
		}

		template< class T >
		void Object::RemoveFirstComponentOfType()
		{
			const auto componentType = ComponentType( typeid( T ) );

			m_components.erase( std::find_if( m_components.begin(), m_components.end(), [&ComponentHandle]( const Handle& componentHandle )
			{
				if( !componentHandle.Get() )
					return false;

				return componentType == ComponentType( typeid( *componentHandle.Get() ) )
			} ) );
		}
	}
}