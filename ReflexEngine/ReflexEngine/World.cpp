
#include "World.h"
#include "Object.h"


namespace Reflex
{
	namespace Core
	{
		World::World( sf::RenderTarget& window )
			: m_window( window )
			, m_worldView( m_window.getDefaultView() )
			, m_components( 10 )
			, m_objects( sizeof( Object ), 100 )
		{
			BaseHandle::s_handleManager = &m_handles;
		}

		World::~World()
		{
			DestroyAllObjects();
		}

		void World::Update( const sf::Time deltaTime )
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

		ObjectHandle World::CreateObject()
		{
			Object* newObject = ( Object* )m_objects.Allocate();
			new ( newObject ) Object( *this, m_handles.Insert( newObject ) );

			SyncHandles< Object >( m_objects );

			return ObjectHandle( newObject->m_self );
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