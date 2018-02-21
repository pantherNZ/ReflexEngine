
#include "World.h"

namespace Reflex
{
	namespace Core
	{
		World::World( sf::RenderTarget& window )
			: mWindow( window )
			, mWorldView( mWindow.getDefaultView() )
			, mComponents( 10 )
			, mObjects( sizeof( Object ), 1000 )
		{
			//BuildScene();
		}

		void World::Update( const sf::Time deltaTime )
		{
			//mWorldGraph.Update( deltaTime );

			// Deleting objects
			if( !mMarkedForDeletion.empty() )
			{
				for( auto& object : mMarkedForDeletion )
					mObjects.Release( &object );

				mMarkedForDeletion.clear();
			}
		}

		void World::Render()
		{
			mWindow.setView( mWorldView );
			//mWindow.draw( mWorldGraph );
		}

		Object& World::CreateObject()
		{
			Object* newObject = ( Object* )mObjects.Allocate();
			new ( newObject ) Object( *this );
			return *newObject;
		}

		void World::DestroyObject( Object& object )
		{
			mMarkedForDeletion.push_back( object );
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