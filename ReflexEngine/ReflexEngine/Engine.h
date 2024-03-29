#pragma once

#define PROFILING

// Includes
#include "Precompiled.h"
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
			void Update( const float deltaTime );
			void Render();

			void UpdateStatistics( const float deltaTime );
		
		protected:
			// Handle manager which maps a handle to a void* in memory (such as in the above object allocator or a component allocator)
			HandleManager m_handleManager;

			// Core window
			sf::RenderWindow m_window;

			// Resource managers
			TextureManager m_textureManager;
			FontManager m_fontManager;

			// State manager (handles different scenes & transitions, contains worlds which hold objects etc.)
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