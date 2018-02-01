#include "Sprite.h"

namespace Reflex
{
	namespace Core
	{
		Sprite::Sprite()
		{

		}

		Sprite::~Sprite()
		{
			al_destroy_bitmap( m_bitmap );
		}

		void Sprite::Initialise( const int size_x, const int size_y )
		{
			m_bitmap = al_create_bitmap( size_x, size_y );
		}

		void Sprite::SetColour( const Colour& colour, ALLEGRO_DISPLAY& display )
		{
			al_set_target_bitmap( m_bitmap );

			al_clear_to_color( al_map_rgb( colour.r, colour.g, colour.b ) );

			al_set_target_bitmap( al_get_backbuffer( &display ) );
		}

		void Sprite::Render()
		{
			al_draw_bitmap( m_bitmap, m_location.x, m_location.y, 0 );
		}

		void Sprite::Update( const float elapsed_time )
		{

		}

		Vector2d& Sprite::GetLocation()
		{
			return m_location;
		}

		void Sprite::SetLocation( const Vector2d location )
		{
			m_location = location;
		}
	}
}