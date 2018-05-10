
#include "World.h"
#include "Object.h"
#include "TransformComponent.h"
#include "RenderSystem.h"

namespace Reflex
{
	namespace Core
	{
		World::World( sf::RenderTarget& window, sf::FloatRect worldBounds, const unsigned initialMaxObjects )
			: m_window( window )
			, m_worldView( m_window.getDefaultView() )
			, m_worldBounds( worldBounds )
			//, m_box2DWorld( b2Vec2( 0.0f, -9.8f ) )
			, m_components( 10 )
			, m_objects( sizeof( Object ), initialMaxObjects )
			, m_tileMap( m_worldBounds )
		{
			Setup();
		}

		World::World( sf::RenderTarget& window, sf::FloatRect worldBounds, const unsigned spacialHashMapSize, const unsigned initialMaxObjects )
			: m_window( window )
			, m_worldView( m_window.getDefaultView() )
			, m_worldBounds( worldBounds )
			//, m_box2DWorld( b2Vec2( 0.0f, -9.8f ) )
			, m_components( 10 )
			, m_objects( sizeof( Object ), initialMaxObjects )
			, m_tileMap( m_worldBounds, spacialHashMapSize )
		{
			Setup();
		}

		World::~World()
		{
			DestroyAllObjects();
		}

		void World::Setup()
		{
			BaseHandle::s_handleManager = &m_handles;
			m_sceneGraphRoot = CreateObject( false )->GetTransform();
			AddSystem< Reflex::Systems::RenderSystem >();
		}

		void World::Update( const float deltaTime )
		{
			// Update systems
			for( auto& system : m_systems )
				system.second->Update( deltaTime );

			// Deleting objects
			DeletePendingItems();
		}

		void World::ProcessEvent( const sf::Event& event )
		{
			for( auto& system : m_systems )
				system.second->ProcessEvent( event );
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
			return CreateObject( true, position, rotation, scale );
		}

		ObjectHandle World::CreateObject( const bool attachToRoot, const sf::Vector2f& position, const float rotation, const sf::Vector2f& scale )
		{
			Object* newObject = ( Object* )m_objects.Allocate();
			new ( newObject ) Object( *this, m_handles.Insert( newObject ) );

			SyncHandles< Object >( m_objects );

			auto newHandle = ObjectHandle( newObject->m_self );
			newHandle->AddComponent< Reflex::Components::Transform >( position, rotation, scale );

			if( attachToRoot )
				m_sceneGraphRoot->AttachChild( newHandle );

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

		ObjectHandle World::GetSceneObject( const unsigned index /*= 0U*/ ) const
		{
			return m_sceneGraphRoot->GetChild( index );
		}
	}
}