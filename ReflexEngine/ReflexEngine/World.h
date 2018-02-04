#pragma once

#include "Common.h"
#include "WorldNode.h"
#include "ResourceManager.h"

// Engine class
namespace Reflex
{
	namespace Core
	{
		template< typename Resource >
		class ResourceManager;

		typedef std::unique_ptr< ResouceManager< sf::Texture > > TextureManager;

		// World class
		class World : private sf::NonCopyable
		{
		public:
			explicit World( sf::RenderTarget& window );

			void Update( const sf::Time deltaTime );
			void Render();

		protected:
			void LoadTextures();
			void BuildScene();

		protected:
			enum Variables
			{
				MaxLayers = 5,
			};

			sf::RenderTarget& m_window;
			sf::View m_worldView;

			sf::FloatRect m_worldBounds;
			WorldNode m_worldGraph;
			std::array< WorldNode*, MaxLayers > m_sceneLayers;

			//std::unique_ptr< SceneManager > m_sceneManager;
			TextureManager m_textureManager;
		};
	}
}