#pragma once

#include "Common.h"
#include "SceneNode.h"
#include "ResourceManager.h"

// Engine class
namespace Reflex
{
	namespace Core
	{
		// World class
		class World : private sf::NonCopyable
		{
		public:
			explicit World( sf::RenderTarget& window );

			void Update( const sf::Time deltaTime );
			void Render();

			SceneNode* GetWorldGraphFromLayer( unsigned short layer ) const;
			void AddSceneNode( unsigned short layer, std::unique_ptr< SceneNode > node );

		protected:
			void BuildScene();

		protected:
			enum Variables
			{
				MaxLayers = 5,
			};

			sf::RenderTarget& m_window;
			sf::View m_worldView;

			//std::vector< std::unique_ptr< SceneNode > > m_worldObjects;
			SceneNode m_worldGraph;
			std::array< SceneNode*, MaxLayers > m_sceneLayers;
		};
	}
}