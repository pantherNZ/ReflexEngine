#pragma once

#include "Common.h"
#include "ResourceManager.h"
#include "Object.h"
#include "ObjectAllocator.h"
#include "System.h"

#include <unordered_map>
#include "HandleManager.h"

// Engine class
namespace Reflex
{
	namespace Core
	{
		using Systems::System;

		// World class
		class World : private sf::NonCopyable
		{
		public:
			explicit World( sf::RenderTarget& window );

			void Update( const sf::Time deltaTime );
			void Render();

			ObjectHandle CreateObject();
			void DestroyObject( ObjectHandle object );

			void AddSystem( std::unique_ptr< System > system );
			
			template< class T, typename... Args >
			void AddSystem( Args&&... args );

			template< class T >
			std::unique_ptr< System > GetSystem();

			template< class T >
			void RemoveSystem();

			template< class T >
			void ForwardRegisterComponent();

			template< class T >
			ComponentHandle CreateComponent();

			//template< class T >
			//void DestroyComponent();

			void DestroyComponent( ComponentHandle component );

			template< class T >
			void SyncHandles( ObjectAllocator& m_array );

			template< class T >
			void SyncHandlesForce( ObjectAllocator& m_array );

			HandleManager& GetHandleManager();

		protected:
			//void BuildScene();

		private:
			World() = delete;

		protected:
			enum Variables
			{
				MaxLayers = 5,
			};

			sf::RenderTarget& m_window;
			sf::View m_worldView;

			ObjectAllocator m_objects;
			HandleManager m_handles;

			std::unordered_map< Type, std::unique_ptr< ObjectAllocator > > m_components;
			std::unordered_map< std::unique_ptr< System >, std::vector< Type > > m_systems;

			std::vector< Type >* m_last_added_system = nullptr;
			std::vector< ObjectHandle > m_markedForDeletion;
		};

		// Template functions
		template< class T, typename... Args >
		void World::AddSystem( Args&&... args )
		{
			AddSystem( std::move( std::make_unique< T >( std::forward< Args >( args )... ) ) );
		}

		/*template<typename T, typename... Args>
		std::unique_ptr<T> make_unique(Args&&... args)
		{
			return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
		}*/

		template< class T >
		void World::RemoveSystem()
		{
			const auto systemType = Type( typeid( T ) );

			for( auto iter = m_systems.begin(); iter != m_systems.end(); ++iter )
			{
				if( systemType == Type( typeid( iter->first.get() ) ) )
				{
					iter->first->OnSystemShutdown();
					iter->first.release();
					m_systems.erase( iter );
					break;
				}
			}
		}

		template< class T >
		std::unique_ptr< System > World::GetSystem()
		{
			const auto systemType = Type( typeid( T ) );

			for( auto iter = m_systems.begin(); iter != m_systems.end(); ++iter )
			{
				if( systemType == Type( typeid( iter->first.get() ) ) )
				{
					auto system = std::move( iter->first );
					m_systems.erase( iter );
					return std::move( system );
				}
			}

			return nullptr;
		}

		template< class T >
		void World::ForwardRegisterComponent()
		{
			const auto componentType = ComponentType( typeid( T ) );

			if( m_components.find( componentType ) == m_components.end() )
				m_components.insert( componentType, std::make_unique< ObjectAllocator >( sizeof( T ), 10 ) );

			if( m_last_added_system )
				m_last_added_system->push_back( componentType );
		}

		template< class T >
		ComponentHandle World::CreateComponent()
		{
			const auto componentType = ComponentType( typeid( T ) );

			auto found = m_components.find( componentType );

			if( found == m_components.end() )
				found = m_components.insert( componentType, std::make_unique< ObjectAllocator >( sizeof( T ), 10 ) );

			auto* component = ( T* )found->second.Allocate();
			new ( component ) T( *this );

			return m_handles.Insert< Component >( component );
		}

		/*template< class T >
		void World::DestroyComponent()
		{
			const auto componentType = ComponentType( typeid( T ) );

			auto found = m_components.find( componentType );

			if( found == m_components.end() )
				return; // This is weird, duno how this could ever happen

			found->second.Release( Handle.Get() );
		}*/

		template< class T >
		void World::SyncHandles( ObjectAllocator& m_array )
		{
			if( m_array.Grew() )
			{
				for( auto i = m_array.begin<T>(); i != m_array.end<T>(); ++i )
					m_handles.Update( &( *i ), i->m_self );
				m_array.ClearGrewFlag();
			}
		}

		template< class T >
		void World::SyncHandlesForce( ObjectAllocator& m_array )
		{
			for( auto i = m_array.begin<T>(); i != m_array.end<T>(); ++i )
				m_handles.Update( &( *i ), i->m_self );
		}
	}
}