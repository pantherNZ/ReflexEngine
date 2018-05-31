
#include "World.h"
#include "Object.h"
#include "TransformComponent.h"
#include "RenderSystem.h"
#include "InteractableSystem.h"

namespace Reflex
{
	namespace Core
	{
		World::World( Context context, sf::FloatRect worldBounds, const unsigned initialMaxObjects )
			: m_context( context )
			, m_worldView( context.window->getDefaultView() )
			, m_worldBounds( worldBounds )
			//, m_box2DWorld( b2Vec2( 0.0f, -9.8f ) )
			, m_components( 10 )
			, m_objects( sizeof( Object ), initialMaxObjects )
			, m_tileMap( m_worldBounds )
		{
			Setup();
		}

		World::World( Context context, sf::FloatRect worldBounds, const unsigned spacialHashMapSize, const unsigned initialMaxObjects )
			: m_context( context )
			, m_worldView( context.window->getDefaultView() )
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
			AddSystem< Reflex::Systems::InteractableSystem >();
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
				for( auto& objectHandle : m_markedForDeletion )
				{
					auto* object = objectHandle.Get();

					// Detach from parent
					const auto parent = object->GetTransform()->GetParent();
					if( parent )
						parent->GetTransform()->DetachChild( objectHandle );

					m_handles.Remove( objectHandle );
					object->RemoveAllComponents();
					object->~Object();
					auto moved = ( Object* )m_objects.Release( object );

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
				auto* object = ( Entity* )allocator[0];
				m_handles.Remove( object->m_self );
				object->~Entity();

				auto moved = ( Entity* )allocator.Release( object );

				// Sync handle of potentially moved object
				if( moved )
					m_handles.Update( moved );
			}
		}

		void World::Render()
		{
			m_context.window->setView( m_worldView );

			for( auto iter = m_systems.begin(); iter != m_systems.end(); ++iter )
			{
				m_context.window->draw( *iter->second );
			}
		}

		ObjectHandle World::CreateObject( const sf::Vector2f& position, const float rotation, const sf::Vector2f& scale )
		{
			return CreateObject( false, position, rotation, scale );
		}

		ObjectHandle World::CreateObject( const bool attachToRoot, const sf::Vector2f& position, const float rotation, const sf::Vector2f& scale )
		{
			// Allocate the component's memory from the allocator
			Object* newObject = ( Object* )m_objects.Allocate();

			// Create handle & construct
			auto objectHandle = m_handles.Insert( newObject );
			new ( newObject ) Object( *this );
			newObject->m_self = objectHandle;

			SyncHandles< Object >( m_objects );

			// Add the transform component by default
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

			for( auto& allocator : m_components )
				ResetAllocator( *allocator.second.get() );

			for( auto& system : m_systems )
				system.second->m_components.clear();
		}

		void World::DestroyComponent( Type componentType, BaseHandle component )
		{
			Entity* entity = m_handles.GetAs< Entity >( component );
			m_handles.Remove( component );
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
			return *m_context.window;
		}

		Context& World::GetContext()
		{
			return m_context;
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

		std::unordered_map< Type, std::unique_ptr< ObjectAllocator > >::iterator World::GetComponentAllocator( const Type& componentType, const size_t componentSize )
		{
			// Create an allocator for this type if one doesn't already exist
			auto found = m_components.find( componentType );

			if( found == m_components.end() )
			{
				const auto result = m_components.insert( std::make_pair( componentType, std::make_unique< ObjectAllocator >( componentSize, 10 ) ) );
				found = result.first;

				if( !result.second )
					LOG_CRIT( "Failed to allocate memory for component allocator of type: " << componentType.name() );
			}

			return found;
		}

		void World::AddComponentToSystems( const ObjectHandle& owner, const BaseHandle& componentHandle, const Type& componentType )
		{
			// Here we want to check if we should add this component to any systems
			for( auto iter = m_systems.begin(); iter != m_systems.end(); ++iter )
			{
				// If the system doesn't care about this type, skip it
				const auto& requiredTypes = iter->second->m_requiredComponentTypes;
				if( std::find( requiredTypes.begin(), requiredTypes.end(), componentType ) == requiredTypes.end() )
					continue;

				auto& componentsPerObject = iter->second->m_components;

				std::vector< BaseHandle > tempList;
				bool canAddDueToNewComponent = false;

				// This looks through the required types and sees if the object has one of each of them
				for( auto& requiredType : requiredTypes )
				{
					const auto handle = ( requiredType == componentType ? componentHandle : owner->GetComponent( requiredType ) );

					if( !handle.IsValid() )
						break;

					if( handle == componentHandle )
						canAddDueToNewComponent = true;

					tempList.push_back( handle );
				}

				// Escape if the object didn't have the components required OR if our new component isn't even the one that is now allowing it to be a part of the system
				if( tempList.size() < requiredTypes.size() || !canAddDueToNewComponent )
					continue;

				iter->second->m_components.push_back( std::move( tempList ) );
			}
		}
	}
}