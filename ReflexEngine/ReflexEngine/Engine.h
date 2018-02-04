#pragma once

// Includes
#include "Common.h"
#include "World.h"

namespace Reflex
{
	namespace Core
	{
		// Forward declarations
		class SceneManager;

		// Engine class
		class Engine : private sf::NonCopyable
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
			void KeyboardInput( const sf::Keyboard::Key key, const bool isPressed );
			void ProcessEvents();
			void Update( const sf::Time deltaTime );
			void Render();

			void UpdateStatistics( const sf::Time deltaTime );
		
		private:
			sf::RenderWindow m_window;
			World m_world;

			const sf::Time m_updateInterval;
			sf::Font m_font;
			sf::Text m_statisticsText;
			sf::Time m_statisticsUpdateTime;
			unsigned int m_statisticsNumFrames = 0U;
		};
	}
}