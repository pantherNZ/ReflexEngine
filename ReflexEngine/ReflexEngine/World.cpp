
#include "World.h"
#include "Object.h"
#include "TransformComponent.h"
#include "RenderSystem.h"

namespace Reflex
{
	namespace Core
	{
		World::World( sf::RenderTarget& window, sf::FloatRect worldBounds, const unsigned tileMapGridSize /*= 0U*/ )
			: m_window( window )
			, m_worldView( m_window.getDefaultView() )
			, m_worldBounds( worldBounds )
			//, m_box2DWorld( b2Vec2( 0.0f, -9.8f ) )
			, m_components( 10 )
			, m_objects( sizeof( Object ), 100 )
			, m_tileMap( m_worldBounds, tileMapGridSize )
		{
			BaseHandle::s_handleManager = &m_handles;

			AddSystem< Reflex::Systems::RenderSystem >();
		}

		World::World( sf::RenderTarget& window, sf::FloatRect worldBounds, const SpacialStorageType type, const unsigned storageSize, const unsigned tileMapGridSize /*= 0U*/ )
			: m_window( window )
			, m_worldView( m_window.getDefaultView() )
			, m_worldBounds( worldBounds )
			//, m_box2DWorld( b2Vec2( 0.0f, -9.8f ) )
			, m_components( 10 )
			, m_objects( sizeof( Object ), 100 )
			, m_tileMap( m_worldBounds, type, storageSize, tileMapGridSize )
		{
			BaseHandle::s_handleManager = &m_handles;

			AddSystem< Reflex::Systems::RenderSystem >();
		}

		World::~World()
		{
			DestroyAllObjects();
		}

		void World::Update( const float deltaTime )
		{
			// Update systems
			for( auto& system : m_systems )
				system.second->Update( deltaTime );

			// Deleting objects
			DeletePendingItems();
		}

		void World::DeletePendingItems()
		{
			if( !m_markedForDeletion.empty() )
			{
				for( auto& entityHandle : m_markedForDeletion )
				{
					Entity* entity = m_handles.GetAs< Entity >( entityHandle );
					m_handles.Remove( entityHandle );
					entity->~Entity();
					auto moved = ( Entity* )m_objects.Release( entity );

					// Sync handle of potentially moved object
					if( moved )
						m_handles.Update( moved );
				}

				m_markedForDeletion.clear();
			}
		}

		void World::ResetAllocator( ObjectAllocator& allocator )
		{
			while( allocator.Size() )
			{
				auto* component = ( Entity* )allocator[0];

				Entity* entity = m_handles.GetAs< Entity >( component->m_self );
				m_handles.Remove( component->m_self );
				entity->~Entity();
				auto moved = ( Entity* )allocator.Release( entity );

				// Sync handle of potentially moved object
				if( moved )
					m_handles.Update( moved );
			}
		}

		void World::Render()
		{
			m_window.setView( m_worldView );

			for( auto iter = m_systems.begin(); iter != m_systems.end(); ++iter )
			{
				m_window.draw( *iter->second );
			}
		}

		ObjectHandle World::CreateObject( const sf::Vector2f& position, const float rotation, const sf::Vector2f& scale )
		{
			Object* newObject = ( Object* )m_objects.Allocate();
			new ( newObject ) Object( *this, m_handles.Insert( newObject ) );

			SyncHandles< Object >( m_objects );

			auto newHandle = ObjectHandle( newObject->m_self );
			newHandle->AddComponent< Reflex::Components::Transform >( position, rotation, scale );
			return newHandle;
		}

		void World::DestroyObject( ObjectHandle object )
		{
			assert( !object.markedForDeletion );
			if( !object.markedForDeletion )
			{
				m_markedForDeletion.push_back( object );
				object.markedForDeletion = true;
			}
		}

		void World::DestroyAllObjects()
		{
			ResetAllocator( m_objects );
			//while( m_objects.Size() )
			//{
			//	auto* object = ( Object* )m_objects[0];
			//	object->Destroy();
			//}

			for( auto& allocator : m_components )
				ResetAllocator( *allocator.second.get() );

			for( auto& system : m_systems )
				system.second->m_components.clear();
		}

		void World::DestroyComponent( Type componentType, BaseHandle component )
		{
			//assert( !component.markedForDeletion );
			//if( !component.markedForDeletion )
			//{
			//	m_markedForDeletion.push_back( component );
			//	component.markedForDeletion = true;
			//
			Entity* entity = m_handles.GetAs< Entity >( component );
			entity->~Entity();
			auto moved = ( Entity* )m_components[componentType]->Release( entity );

			// Sync handle of potentially moved object
			if( moved )
				m_handles.Update( moved );

			// Remove this component from any systems
			for( auto iter = m_systems.begin(); iter != m_systems.end(); ++iter )
			{
				const auto& requiredComponents = iter->second->m_requiredComponentTypes;
				
				if( std::find( requiredComponents.begin(), requiredComponents.end(), componentType ) == requiredComponents.end() )
					continue;

				auto& componentsPerObject = iter->second->m_components;

				componentsPerObject.erase( std::remove_if( componentsPerObject.begin(), componentsPerObject.end(), [&component]( std::vector< BaseHandle >& components )
				{
					return std::find( components.begin(), components.end(), component ) != components.end();
				} 
				), componentsPerObject.end() );
			}
		}

		HandleManager& World::GetHandleManager()
		{
			return m_handles;
		}

		sf::RenderTarget& World::GetWindow()
		{
			return m_window;
		}

		Reflex::Core::TileMap& World::GetTileMap()
		{
			return m_tileMap;
		}

		const sf::FloatRect World::GetBounds() const
		{
			return m_worldBounds;
		}

		//Reflex::Core::SceneNode* World::GetWorldGraphFromLayer( unsigned short layer ) const
		//{
		//	return mSceneLayers[layer];
		//}
		//
		//void World::AddSceneNode( unsigned short layer, std::unique_ptr< SceneNode > node )
		//{
		//	m_sceneLayers[layer]->AttachChild( std::move( node ) );
		//}
		//
		//void World::BuildScene()
		//{
		//	// Initialize the different layers
		//	for( std::size_t i = 0; i < MaxLayers; ++i )
		//	{
		//		auto layerNode = std::make_unique< SceneNode >();
		//		m_sceneLayers[i] = layerNode.get();
		//		m_worldGraph.AttachChild( std::move( layerNode ) );
		//	}
		//}
	}
}