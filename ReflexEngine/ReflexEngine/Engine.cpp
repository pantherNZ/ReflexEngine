// Includes
#include "Engine.h"

// Implementation
namespace Reflex
{
	namespace Core
	{
		Engine::Engine()
			: m_updateInterval( sf::seconds( 1.0f / 60.f ) )
			, m_window( sf::VideoMode::getFullscreenModes()[0], "ReflexEngine", sf::Style::Default )
			, m_textureManager()
			, m_fontManager()
			, m_stateManager( Context( m_window, m_textureManager, m_fontManager ) )
		{
			// 2560, 1377
			m_window.setPosition( sf::Vector2i( -6, 0 ) );

			m_font.loadFromFile( "Data/Fonts/arial.ttf" );
			m_statisticsText.setFont( m_font );
			m_statisticsText.setPosition( 5.0f, 5.0f );
			m_statisticsText.setCharacterSize( 15 );
		}

		Engine::~Engine()
		{
		}

		void Engine::Run()
		{
			try
			{
				sf::Clock clock;
				sf::Time accumlatedTime = sf::Time::Zero;

				while( m_window.isOpen() )
				{
					ProcessEvents();
					sf::Time deltaTime = clock.restart();
					accumlatedTime += deltaTime;
					bool updated = false;

					while( accumlatedTime > m_updateInterval )
					{
						accumlatedTime -= m_updateInterval;
						ProcessEvents();
						Update( m_updateInterval );
						updated = true;
					}

					UpdateStatistics( deltaTime );
					Render();
				}
			}
			catch( std::exception& e )
			{
				Reflex::LOG_CRIT( Stream( "EXCEPTION: " << *e.what() << "\n" ) );
			}
		}

		void Engine::SetStartupState( const unsigned stateID )
		{
			m_stateManager.PushState( stateID );
		}

		void Engine::ProcessEvents()
		{
			sf::Event event;

			while( m_window.pollEvent( event ) )
			{
				m_stateManager.ProcessEvent( event );

				switch( event.type )
				{
				case sf::Event::KeyPressed:
					KeyboardInput( event.key.code, true );
					break;

				case sf::Event::KeyReleased:
					KeyboardInput( event.key.code, false );
					break;

				case sf::Event::Closed:
					m_window.close();
					break;
				default: break;
				}
			}
		}

		void Engine::KeyboardInput( const sf::Keyboard::Key key, const bool isPressed )
		{
			if( key == sf::Keyboard::Escape )
				m_window.close();
		}

		void Engine::Update( const sf::Time deltaTime )
		{
			m_stateManager.Update( deltaTime );
		}

		void Engine::Render()
		{
			m_window.clear( sf::Color::Black );
			m_stateManager.Render();
			m_window.setView( m_window.getDefaultView() );
			m_window.draw( m_statisticsText );
			m_window.display();
		}

		void Engine::UpdateStatistics( const sf::Time deltaTime )
		{
			m_statisticsUpdateTime += deltaTime;
			m_statisticsNumFrames += 1;

			if( m_statisticsUpdateTime >= sf::seconds( 1.0f ) )
			{
				const auto ms_per_frame = m_statisticsUpdateTime.asMilliseconds() / m_statisticsNumFrames;
				m_statisticsText.setString(
					"FPS: " + std::to_string( m_statisticsNumFrames ) + "\n" +
					"Frame Time: " + ( ms_per_frame > 0 ? std::to_string( ms_per_frame ) + "ms" :
					std::to_string( m_statisticsUpdateTime.asMicroseconds() / m_statisticsNumFrames ) + "us" ) );

				m_statisticsUpdateTime -= sf::seconds( 1.0f );
				m_statisticsNumFrames = 0;
			}
		}
	}
}