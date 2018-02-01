// Includes
#include "Window.h"

#include <allegro5/display.h>

// Implementation
namespace Reflex
{
	namespace Core
	{
		Window::Window()
		{

		}

		Window::~Window()
		{
			if( m_display )
				al_destroy_display( m_display );
		}

		bool Window::Initialise()
		{
			if( !al_init() )
			{
				fprintf( stderr, "Failed to initialize allegro!\n" );
				return false;
			}
			
			ALLEGRO_DISPLAY_MODE disp_data = GetMaxResolution();
			m_display = al_create_display( disp_data.width, disp_data.height );
			al_set_window_position( m_display, 0, 0 );

			if( !m_display )
			{
				fprintf( stderr, "Failed to create display!\n" );
				return false;
			}

			if( m_fullscreen )
				SetFullscreen( true, true );

			return true;
		}

		void Window::RenderBegin()
		{
			al_clear_to_color( al_map_rgb( 0, 0, 0 ) );
		}

		void Window::RenderEnd()
		{
			al_flip_display();
		}

		ALLEGRO_DISPLAY* Window::GetDisplay() const
		{
			return m_display;
		}

		ALLEGRO_DISPLAY_MODE Window::GetMaxResolution() const
		{
			ALLEGRO_DISPLAY_MODE disp_data;
			al_get_display_mode( 0, &disp_data );
			return disp_data;
		}

		int Window::GetWidth() const
		{
			return al_get_display_width( m_display );
		}

		int Window::GetHeight() const
		{
			return al_get_display_height( m_display );
		}

		bool Window::SetFullscreen( const bool fullscreen, const bool force /*= false*/ )
		{
			if( force || fullscreen != m_fullscreen )
			{
				al_set_display_flag( m_display, ALLEGRO_FULLSCREEN_WINDOW, fullscreen );
				m_fullscreen = fullscreen;
				return true;
			}

			return false;
		}
	}
}