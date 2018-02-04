
#include "World.h"

namespace Reflex
{
	namespace Core
	{
		World::World( sf::RenderTarget& window )
			: m_window( window )
			, m_worldView( m_window.getDefaultView() )
			, m_textureManager( std::make_unique< ResouceManager< sf::Texture > >() )
		{
			//m_sceneManager = std::make_unique< SceneManager >();
			LoadTextures();

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

		void World::LoadTextures()
		{

		}

		void World::BuildScene()
		{
			// Initialize the different layers
			for( std::size_t i = 0; i < MaxLayers; ++i )
			{
				auto layerNode = std::make_unique< WorldNode >();
				m_sceneLayers[i] = layerNode.get();
				m_worldGraph.AttachChild( std::move( layerNode ) );
			}
		}
	}
}