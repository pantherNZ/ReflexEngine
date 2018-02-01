#pragma once

#include "CommonIncludes.h"
// https://wiki.allegro.cc/index.php?title=Allegro_5_Tutorial/Bitmaps

namespace Reflex
{
	namespace Core
	{
		class Sprite
		{
		public:
			Sprite();
			~Sprite();
			void Initialise( const int size_x, const int size_y );
			void SetColour( const Colour& colour, ALLEGRO_DISPLAY& display );
			void Render();
			void Update( const float elapsed_time );
			Vector2d& GetLocation();
			void SetLocation( const Vector2d location );

		private:
			ALLEGRO_BITMAP* m_bitmap = nullptr;
			Vector2d m_location;
		};
	}
}
