#pragma once

// Includes
#include "CommonIncludes.h"

#include <SFML/Graphics.hpp>

// Forward declarations
namespace Reflex
{
	namespace Core
	{
	}
}

// Engine class
namespace Reflex
{
	namespace Core
	{
		class Engine
		{
		public:
			Engine();
			~Engine();

			void Run();

		protected:
			//virtual SceneId GetStartupScene() const = 0;
			//virtual void RegisterScenes( std::vector< Scene > scenes() = 0;
			//virtual void OnPostSetup() { }

		private:
			void KeyboardInput( const sf::Keyboard::Key key, const bool is_pressed );
			void ProcessEvents();
			void Update( const sf::Time delta_time );
			void Render();
		
			sf::RenderWindow m_window;
			const sf::Time m_update_interval;


			sf::Sprite m_sprite;
			sf::Texture m_texture;
		};
	}
}