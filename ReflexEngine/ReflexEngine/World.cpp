
#include "World.h"

namespace Reflex
{
	namespace Core
	{
		World::World( sf::RenderTarget& window )
			: m_window( window )
			, m_worldView( m_window.getDefaultView() )
		{
			BuildScene();
		}

		void World::Update( const sf::Time deltaTime )
		{
			m_worldGraph.Update( deltaTime );
		}

		void World::Render()
		{
			m_window.setView( m_worldView );
			m_window.draw( m_worldGraph );
		}

		Reflex::Core::SceneNode* World::GetWorldGraphFromLayer( unsigned short layer ) const
		{
			return m_sceneLayers[layer];
		}

		void World::AddSceneNode( unsigned short layer, std::unique_ptr< SceneNode > node )
		{
			m_sceneLayers[layer]->AttachChild( std::move( node ) );
		}

		void World::BuildScene()
		{
			// Initialize the different layers
			for( std::size_t i = 0; i < MaxLayers; ++i )
			{
				auto layerNode = std::make_unique< SceneNode >();
				m_sceneLayers[i] = layerNode.get();
				m_worldGraph.AttachChild( std::move( layerNode ) );
			}
		}
	}
}