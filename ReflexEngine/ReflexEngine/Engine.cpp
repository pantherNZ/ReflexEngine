// Includes
#include "Engine.h"

// Implementation
namespace Reflex
{
	namespace Core
	{
		Engine::Engine() 
			: m_update_interval( sf::seconds( 1.0f / 60.f ) )
		{
			// 2560, 1377
			m_window.create( sf::VideoMode( 640, 300 ), "ReflexEngine", sf::Style::Default );
			//m_window.setPosition( sf::Vector2i( -6, 0 ) );

			if( !m_texture.loadFromFile( "Data/Textures/3zlcnZu.jpg" ) )
			{
				Log( ELogType::CRIT, "Texture failed to load" );
				return;
			}

			m_sprite.setTexture( m_texture );
			m_sprite.setPosition( 100.0f, 100.0f );
		}

		Engine::~Engine()
		{
		}

		void Engine::Run()
		{
			sf::Clock clock;
			sf::Time accumlated_time = sf::Time::Zero;

			while( m_window.isOpen() )
			{
				ProcessEvents();
				accumlated_time += clock.restart();
				while( accumlated_time > m_update_interval )
				{
					accumlated_time -= m_update_interval;
					ProcessEvents();
					Update( m_update_interval );
				}

				const auto pos = m_window.getPosition();
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

		void Engine::KeyboardInput( const sf::Keyboard::Key key, const bool is_pressed )
		{
			if( key == sf::Keyboard::Escape )
				m_window.close();
		}

		void Engine::Update( const sf::Time delta_time )
		{

		}

		void Engine::Render()
		{
			m_window.clear();

			m_window.draw( m_sprite );

			m_window.display();
		}
	}
}