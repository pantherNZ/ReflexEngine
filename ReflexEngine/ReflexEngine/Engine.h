#pragma once

// Includes
#include "Common.h"
#include "World.h"
#include "StateManager.h"
#include "ResourceManager.h"

namespace Reflex
{
	namespace Core
	{
		// Engine class
		class Engine : private sf::NonCopyable
		{
		public:
			Engine();
			~Engine();

			void Run();

			template< typename T >
			void RegisterState( const unsigned stateID, const bool isStartingState = false );

			void SetStartupState( const unsigned stateID );

		protected:
			void KeyboardInput( const sf::Keyboard::Key key, const bool isPressed );
			void ProcessEvents();
			void Update( const sf::Time deltaTime );
			void Render();

			void UpdateStatistics( const sf::Time deltaTime );
		
		protected:
			sf::RenderWindow m_window;
			TextureManager m_textureManager;
			FontManager m_fontManager;
			StateManager m_stateManager;

			const sf::Time m_updateInterval;
			sf::Font m_font;
			sf::Text m_statisticsText;
			sf::Time m_statisticsUpdateTime;
			unsigned int m_statisticsNumFrames = 0U;
		};

		template< typename T >
		void Engine::RegisterState( const unsigned stateID, const bool isStartingState )
		{
			m_stateManager.RegisterState< T >( stateID );

			if( isStartingState )
				SetStartupState( stateID );
		}
	}
}