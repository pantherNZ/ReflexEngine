#pragma once

#include "Common.h"
#include "ResourceManager.h"
#include "Object.h"
#include "ObjectAllocator.h"
#include "System.h"
#include "HandleManager.h"

#include <unordered_map>

// Engine class
namespace Reflex
{
	namespace Core
	{
		using Systems::System;
		using Reflex::Components::Component;

		// World class
		class World : private sf::NonCopyable
		{
		public:
			explicit World( sf::RenderTarget& window );
			~World();

			void Update( const sf::Time deltaTime );
			void Render();

			ObjectHandle CreateObject();
			void DestroyObject( ObjectHandle object );

			void AddSystem( std::unique_ptr< System > system );
			
			template< class T, typename... Args >
			void AddSystem( Args&&... args );

			template< class T >
			System* GetSystem();

			template< class T >
			void RemoveSystem();

			template< class T >
			void ForwardRegisterComponent();

			template< class T, typename... Args >
			Handle< T > CreateComponent( ObjectHandle& owner, Args&&... args );

			template< class T >
			void DestroyComponent( Handle< T > component );

			template< class T >
			void SyncHandles( ObjectAllocator& m_array );

			template< class T >
			void SyncHandlesForce( ObjectAllocator& m_array );

			HandleManager& GetHandleManager();
			sf::RenderTarget& GetWindow();

		protected:
			//void BuildScene();
			void DeletePendingItems();

		private:
			World() = delete;

		protected:
			enum Variables
			{
				MaxLayers = 5,
			};

			sf::RenderTarget& m_window;
			sf::View m_worldView;

			// Storage for all objects in the game
			ObjectAllocator m_objects;

			// Handle manager which maps a handle to a void* in memory (such as in the above object allocator or a component allocator)
			HandleManager m_handles;

			// List of components, indexed by their type (EG. Sprite), this is what holds the memory of all components in the engine
			std::unordered_map< Type, std::unique_ptr< ObjectAllocator > > m_components;

			// List of systems, indexed by their pointer, which grants access to the vector storing the component types it requires
			std::unordered_map< std::unique_ptr< System >, std::vector< Type > > m_systems;

			// Internal pointer, used to add required component types to the last added system (in AddSystem call)
			std::vector< Type >* m_newSystemRequiredComponents = nullptr;

			// Removes objects / components on frame move instead of during sometime dangerous
			std::vector< BaseHandle > m_markedForDeletion;
		};

		// Template functions
		template< class T, typename... Args >
		void World::AddSystem( Args&&... args )
		{
			AddSystem( std::move( std::make_unique< T >( *this, std::forward< Args >( args )... ) ) );
		}

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
		System* World::GetSystem()
		{
			const auto systemType = Type( typeid( T ) );

			for( auto iter = m_systems.begin(); iter != m_systems.end(); ++iter )
			{
				if( systemType == Type( typeid( iter->first.get() ) ) )
				{
					return iter->first.get();
				}
			}

			return nullptr;
		}

		template< class T >
		void World::ForwardRegisterComponent()
		{
			const auto componentType = Type( typeid( T ) );

			if( m_components.find( componentType ) == m_components.end() )
				m_components.insert( std::make_pair( componentType, std::make_unique< ObjectAllocator >( sizeof( T ), 10 ) ) );

			if( m_newSystemRequiredComponents )
				m_newSystemRequiredComponents->push_back( componentType );
		}

		template< class T, typename... Args >
		Handle< T > World::CreateComponent( ObjectHandle& owner, Args&&... args )
		{
			const auto componentType = Type( typeid( T ) );

			// Create an allocator for this type if one doesn't already exist
			auto found = m_components.find( componentType );

			if( found == m_components.end() )
			{
				const auto result = m_components.insert( std::make_pair( componentType, std::make_unique< ObjectAllocator >( sizeof( T ), 10 ) ) );
				found = result.first;
			
				if( !result.second )
				{
					LOG_CRIT( Stream( "Failed to allocate memory for component allocator of type: " << componentType.name() ) );
					return Handle< T >();
				}
			}

			// Allocate the object from the allocator
			auto* component = ( T* )found->second->Allocate();

			// Create handle
			const auto componentHandle = m_handles.Insert< T >( component );

			new ( component ) T( owner, componentHandle, std::forward< Args >( args )... );

			SyncHandles< T >( *found->second.get() );

			// Here we want to check if we should add this component to any systems
			for( auto iter = m_systems.begin(); iter != m_systems.end(); ++iter )
			{
				// If the system doesn't care about this type, skip it
				if( std::find( iter->second.begin(), iter->second.end(), componentType ) == iter->second.end() )
					continue;

				auto& componentsPerObject = iter->first->m_components;

				std::vector< BaseHandle > tempList;
				bool canAddDueToNewComponent = false;

				// This looks through the required types and sees if the object has one of each of them
				for( auto& requiredType : iter->second )
				{
					const auto handle = ( requiredType == componentType ? componentHandle : owner->GetComponent( requiredType ) );

					if( !handle.IsValid() )
						break;

					if( handle == componentHandle )
						canAddDueToNewComponent = true;

					tempList.push_back( handle );
				}

				// Escape if the object didn't have the components required OR if our new component isn't even the one that is now allowing it to be a part of the system
				if( tempList.size() < iter->second.size() || !canAddDueToNewComponent )
					continue;

				iter->first->m_components.push_back( std::move( tempList ) );
			}

			return componentHandle;
		}

		template< class T >
		void World::DestroyComponent( Handle< T > component )
		{
			const auto componentType = Type( typeid( *component.Get() ) );
			component.Get()->~T();
			auto moved = ( Entity* )m_components[componentType]->Release( component->Get() );

			// Sync handle of potentially moved object
			if( moved )
				m_handles.Update( moved );

			// Remove this component from any systems
			for( auto iter = m_systems.begin(); iter != m_systems.end(); ++iter )
			{
				if( std::find( iter->second.begin(), iter->second.end(), componentType ) == iter->second.end() )
					continue;

				auto& componentsPerObject = iter->first->m_components;

				std::remove_if( componentsPerObject.begin(), componentsPerObject.end(), [&component]( std::vector< BaseHandle >& components )
				{
					return std::find( components.begin(), components.end(), component ) != components.end();
				} );
			}
		}

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