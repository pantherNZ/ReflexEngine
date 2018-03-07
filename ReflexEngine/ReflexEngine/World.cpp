
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
			, m_objects( sizeof( Object ), 1000 )
		{
			BaseHandle::s_handleManager = &m_handles;
		}

		void World::Update( const sf::Time deltaTime )
		{
			// Update systems
			for( auto& system : m_systems )
				system.first->Update( deltaTime );

			// Deleting objects
			DeletePendingItems();
		}

		void World::DeletePendingItems()
		{
			if( !m_markedForDeletion.empty() )
			{
				for( auto& entityHandle : m_markedForDeletion )
				{
					auto moved = ( Entity* )m_objects.Release( m_handles.Get( entityHandle ) );

					// Sync handle of potentially moved object
					if( moved )
						m_handles.Update( moved );
				}

				m_markedForDeletion.clear();
			}
		}

		void World::Render()
		{
			m_window.setView( m_worldView );

			for( auto iter = m_systems.begin(); iter != m_systems.end(); ++iter )
			{
				m_window.draw( *iter->first );
			}
		}

		ObjectHandle World::CreateObject()
		{
			Object* newObject = ( Object* )m_objects.Allocate();
			new ( newObject ) Object( *this, m_handles.Insert( newObject ) );

			SyncHandles< Object >( m_objects );

			return ObjectHandle( newObject->m_self );
		}

		void World::DestroyObject( BaseHandle object )
		{
			m_markedForDeletion.push_back( object );
		}

		void World::AddSystem( std::unique_ptr< System > system )
		{
			// Insert the new system
			std::vector< Type > requiredComponentTypes;

			// Register components (m_newSystemRequiredComponents allows the system to add required component types to the above created vector)
			m_newSystemRequiredComponents = &requiredComponentTypes;
			system->RegisterComponents();

			// Look for any existing objects that match what this system requires and add them to the system's list
			for( auto object = m_objects.begin< Object >(); object != m_objects.end< Object >(); ++object )
			{
				std::vector< BaseHandle > tempList;

				for( auto& requiredType : *m_newSystemRequiredComponents )
				{
					const auto handle = object->GetComponentOfType( requiredType );

					if( !handle )
						break;
					
					tempList.push_back( handle );
				}

				if( tempList.size() < m_newSystemRequiredComponents->size() )
					continue;

				system->m_components.push_back( std::move( tempList ) );
			}

			m_newSystemRequiredComponents = nullptr;

			auto result = m_systems.insert( std::make_pair( std::move( system ), std::move( requiredComponentTypes ) ) );
			assert( result.second );

			result.first->first->OnSystemStartup();
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