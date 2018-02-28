
#include "World.h"
#include <unordered_set>

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
			//BuildScene();
			ObjectHandle::s_handleManager = &m_handles;
			ComponentHandle::s_handleManager = &m_handles;
		}

		void World::Update( const sf::Time deltaTime )
		{
			// Update systems
			for( auto& system : m_systems )
				system.first->Update( deltaTime );

			// Deleting objects
			if( !m_markedForDeletion.empty() )
			{
				for( auto& objectHandle : m_markedForDeletion )
				{
					objectHandle->m_active = false;
					auto moved = ( Object* )m_objects.Release( objectHandle.Get() );

					if( moved )
						m_handles.Update( moved, moved->m_self );
				}

				m_markedForDeletion.clear();
			}
		}

		void World::Render()
		{
			m_window.setView( m_worldView );
			//mWindow.draw( mWorldGraph );
		}

		ObjectHandle World::CreateObject()
		{
			Object* newObject = ( Object* )m_objects.Allocate();
			new ( newObject ) Object( *this, m_handles.Insert< Object >( newObject ));
			newObject->m_active = true;

			SyncHandles< Object >( m_objects );

			return newObject->m_self;
		}

		void World::DestroyObject( ObjectHandle object )
		{
			m_markedForDeletion.push_back( object );
		}

		void World::AddSystem( std::unique_ptr< System > system )
		{
			// Insert the new system
			auto result = m_systems.insert( std::make_pair( std::move( system ), std::vector< Type >() ) );
			assert( result.second );

			// Register components (m_last_added_system allows the system to add required component types to the above created vector)
			m_last_added_system = &result.first->second;
			system->RegisterComponents();
			m_last_added_system = nullptr;

			// Look for any existing objects that match what this system requires and add them to the system's list
			std::unordered_set< Type > found;

			for( auto i = m_objects.begin< Object >(); i != m_objects.end< Object >(); ++i )
			{
				//i->
				//_components.begin(), m_components.end(), [&componentType]( const ComponentHandle& componentHandle )
				//{
				//	if( !componentHandle.Get() )
				//		return false;
				//
				//	return componentType == ComponentType( typeid( *componentHandle.Get() ) )
				//}
			}

			system->OnSystemStartup();
		}

		void World::DestroyComponent( ComponentHandle component )
		{
			auto* moved = ( Object* )m_objects.Release( component.Get() );

			// Sync handle of potentially moved object
			if( moved )
				m_handles.Update( moved, moved->m_self );
		}

		HandleManager& World::GetHandleManager()
		{
			return m_handles;
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