// Includes
#include "Engine.h"

// Implementation
namespace Reflex
{
	namespace Core
	{
		Engine::Engine()
			: m_updateInterval( sf::seconds( 1.0f / 60.f ) )
			, m_window( sf::VideoMode( 640, 300 ), "ReflexEngine", sf::Style::Default )
			, m_world( m_window )
		{
			// 2560, 1377
			//m_window.setPosition( sf::Vector2i( -6, 0 ) );

			m_font.loadFromFile( "arial.ttf" );
			m_statisticsText.setFont( m_font );
			m_statisticsText.setPosition( 5.0f, 5.0f );
			m_statisticsText.setCharacterSize( 10 );
		}

		Engine::~Engine()
		{
		}

		void Engine::Run()
		{
			sf::Clock clock;
			sf::Time accumlatedTime = sf::Time::Zero;

			while( m_window.isOpen() )
			{
				ProcessEvents();
				sf::Time deltaTime = clock.restart();
				accumlatedTime += deltaTime;

				while( accumlatedTime > m_updateInterval )
				{
					accumlatedTime -= m_updateInterval;
					ProcessEvents();
					Update( m_updateInterval );
				}

				UpdateStatistics( deltaTime );
				Render();
			}
		}

		void Engine::ProcessEvents()
		{
			sf::Event event;

			while( m_window.pollEvent( event ) )
			{
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
			m_world.Update( deltaTime );
		}

		void Engine::Render()
		{
			m_window.clear();
			m_world.Render();
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
				m_statisticsText.setString(
					"Frames / Second = " + std::to_string( m_statisticsNumFrames ) + "\n" +
					"Time / Update = " + std::to_string( m_statisticsUpdateTime.asMicroseconds() / m_statisticsNumFrames ) + "us" );

				m_statisticsUpdateTime -= sf::seconds( 1.0f );
				m_statisticsNumFrames = 0;
			}
		}
	}
}