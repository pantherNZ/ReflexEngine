// Includes
#include "Engine.h"
#include "Window.h"
#include "Sprite.h"

// Implementation
namespace Reflex
{
	namespace Core
	{
		Engine::Engine()
		{

		}

		Engine::~Engine()
		{
			if( m_event_queue )
				al_destroy_event_queue( m_event_queue );

			if( m_timer )
				al_destroy_timer( m_timer );
		}

		bool Engine::Initialise()
		{
			m_window = std::make_unique< Window >();

			if( !m_window->Initialise() )
			{
				fprintf( stderr, "Failed to initialise Window class!\n" );
				return false;
			}

			m_event_queue = al_create_event_queue();

			if( !m_event_queue )
			{
				fprintf( stderr, "Failed to create event_queue!\n" );
				return false;
			}

			m_timer = al_create_timer( 1.0 / 60.0f );

			if( !m_timer )
			{
				fprintf( stderr, "Failed to create timer!\n" );
				return -1;
			}

			al_start_timer( m_timer );

			al_register_event_source( m_event_queue, al_get_display_event_source( m_window->GetDisplay() ) );
			al_register_event_source( m_event_queue, al_get_timer_event_source( m_timer ) );

			m_sprite = std::make_unique< Sprite >();
			m_sprite->Initialise( 32, 32 );
			m_sprite->SetLocation( Vector2d( m_window->GetWidth() / 2.0f - 16.0f, m_window->GetHeight() / 2.0f - 16.0f ) );
			m_sprite->SetColour( Colour( 255.0f, 255.0f, 255.0f ), *m_window->GetDisplay() );
			velocity = Vector2d( -4.0f, 4.0f );

			return true;
		}

		void Engine::Run()
		{
			bool redraw = true;

			while( true )
			{
				ALLEGRO_EVENT ev;
				al_wait_for_event( m_event_queue, &ev );

				if( ev.type == ALLEGRO_EVENT_TIMER )
				{
					auto& location = m_sprite->GetLocation();
					if( location.x < 0 || location.x > m_window->GetWidth() - 32 )
					{
						velocity.x = -velocity.x;
					}

					if( location.y < 0 || location.y > m_window->GetHeight() - 32 )
					{
						velocity.y = -velocity.y;
					}

					location += velocity;

					redraw = true;
				}
				else if( ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE )
					break;

				if( redraw && al_is_event_queue_empty( m_event_queue ) )
				{
					redraw = false;
					m_window->RenderBegin();
					m_sprite->Render();
					m_window->RenderEnd();
				}
			}
		}
	}
}