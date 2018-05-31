#pragma once

#include "Precompiled.h"
#include "ResourceManager.h"
#include "Object.h"
#include "ObjectAllocator.h"
#include "System.h"
#include "HandleFwd.hpp"
#include "TileMap.h"
#include "SceneNode.h"
#include "Context.h"
#include <assert.h>

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
			friend class Object;
			friend class Reflex::Components::Grid;

			explicit World( Context context, sf::FloatRect worldBounds, const unsigned initialMaxObjects );
			explicit World( Context context, sf::FloatRect worldBounds, const unsigned spacialHashMapSize, const unsigned initialMaxObjects );
			~World();

			void Setup();
			void Update( const float deltaTime );
			void ProcessEvent( const sf::Event& event );
			void Render();

			ObjectHandle CreateObject( const sf::Vector2f& position = sf::Vector2f(), const float rotation = 0.0f, const sf::Vector2f& scale = sf::Vector2f( 1.0f, 1.0f ) );
			ObjectHandle CreateObject( const bool attachToRoot, const sf::Vector2f& position = sf::Vector2f(), const float rotation = 0.0f, const sf::Vector2f& scale = sf::Vector2f( 1.0f, 1.0f ) );

			void DestroyObject( ObjectHandle object );

			void DestroyAllObjects();

			template< class T, typename... Args >
			T* AddSystem( Args&&... args );

			template< class T >
			T* GetSystem();

			template< class T >
			void RemoveSystem();

			template< class T >
			void ForwardRegisterComponent();

			template< class T >
			void SyncHandles( ObjectAllocator& m_array );

			template< class T >
			void SyncHandlesForce( ObjectAllocator& m_array );

			template< typename Func >
			void ForEachObject( Func function );

			HandleManager& GetHandleManager();
			sf::RenderTarget& GetWindow();
			Context& GetContext();
			TileMap& GetTileMap();
			const sf::FloatRect GetBounds() const;
			ObjectHandle GetSceneObject( const unsigned index = 0U ) const;

		protected:
			template< class T, typename... Args >
			Handle< T > CreateComponent( const ObjectHandle& owner, Args&&... args );

			void DestroyComponent( Type componentType, BaseHandle component );

			template< class T >
			void DestroyComponent( Handle< T > component );

			std::unordered_map< Type, std::unique_ptr< ObjectAllocator > >::iterator GetComponentAllocator( const Type& componentType, const size_t componentSize );
			void AddComponentToSystems( const ObjectHandle& owner, const BaseHandle& componentHandle, const Type& componentType );

		private:
			World() = delete;

			void DeletePendingItems();
			void ResetAllocator( ObjectAllocator& allocator );

		protected:
			enum Variables
			{
				MaxLayers = 5,
			};

			Context m_context;
			//sf::RenderTarget& m_window;
			sf::View m_worldView;
			sf::FloatRect m_worldBounds;

			//b2World m_box2DWorld;

			// Storage for all objects in the game
			ObjectAllocator m_objects;

			// Handle manager which maps a handle to a void* in memory (such as in the above object allocator or a component allocator)
			HandleManager m_handles;

			// List of components, indexed by their type (EG. Sprite), holds the memory of all components
			std::unordered_map< Type, std::unique_ptr< ObjectAllocator > > m_components;

			// List of systems, indexed by their type, holds memory for all the Systems
			std::unordered_map< Type, std::unique_ptr< System > > m_systems;

			// Tilemap which stores object handles in the world in an efficient spacial hash map
			TileMap m_tileMap;
			TransformHandle m_sceneGraphRoot;

			// Removes objects / components on frame move instead of during sometime dangerous
			std::vector< ObjectHandle > m_markedForDeletion;
		};

		// Template functions
		template< class T, typename... Args >
		T* World::AddSystem( Args&&... args )
		{
			const auto type = Type( typeid( T ) );

			if( m_systems.find( type ) != m_systems.end() )
			{
				LOG_CRIT( "Trying to add a system that has already been added!" );
				return nullptr;
			}

			auto system = std::make_unique< T >( *this, std::forward< Args >( args )... );

			std::vector< Type > requiredComponentTypes;

			// Register components
			system->RegisterComponents();

			// Look for any existing objects that match what this system requires and add them to the system's list
			for( auto object = m_objects.begin< Object >(); object != m_objects.end< Object >(); ++object )
			{
				std::vector< BaseHandle > tempList;

				for( auto& requiredType : system->m_requiredComponentTypes )
				{
					const auto handle = object->GetComponent( requiredType );

					if( !handle )
						break;

					tempList.push_back( handle );
				}

				if( tempList.size() < system->m_requiredComponentTypes.size() )
					continue;

				system->m_components.push_back( std::move( tempList ) );
			}

			auto result = m_systems.insert( std::make_pair( type, std::move( system ) ) );
			assert( result.second );

			result.first->second->OnSystemStartup();

			return ( T* )result.first->second.get();
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
		T* World::GetSystem()
		{
			const auto systemType = Type( typeid( T ) );

			for( auto iter = m_systems.begin(); iter != m_systems.end(); ++iter )
				if( systemType == iter->first )
					return ( T* )iter->second.get();

			return nullptr;
		}

		template< class T >
		void World::ForwardRegisterComponent()
		{
			const auto componentType = Type( typeid( T ) );

			if( m_components.find( componentType ) == m_components.end() )
				m_components.insert( std::make_pair( componentType, std::make_unique< ObjectAllocator >( sizeof( T ), 10 ) ) );
		}

		template< class T, typename... Args >
		Handle< T > World::CreateComponent( const ObjectHandle& owner, Args&&... args )
		{
			const auto componentType = Type( typeid( T ) );
			const auto found = GetComponentAllocator( componentType, sizeof( T ) );

			// This should never happen
			assert( found != m_components.end() );

			// Allocate the component's memory from the allocator
			auto* component = ( T* )found->second->Allocate();

			// Create handle & construct
			const auto componentHandle = m_handles.Insert< T >( component );
			new ( component ) T( std::forward< Args >( args )... );
			component->m_self = componentHandle;
			component->SetOwningObject( owner );

			SyncHandles< T >( *found->second.get() );

			// Here we want to check if we should add this component to any systems
			AddComponentToSystems( owner, componentHandle, componentType );

			return componentHandle;
		}

		template< class T >
		void World::DestroyComponent( Handle< T > component )
		{
			const auto componentType = Type( typeid( *component.Get() ) );
			DestroyComponent( componentType, component );
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

		template< typename Func >
		void World::ForEachObject( Func function )
		{
			for( unsigned i = 0; i < m_objects.Size(); ++i )
				function( ( Object* )m_objects[i] );
		}
	}
}