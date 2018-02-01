#pragma once

namespace Reflex
{
	namespace Core
	{
		struct Vector2d
		{
			float x;
			float y;

			Vector2d() : x( 0.0f), y( 0.0f ) { }
			Vector2d( const float _x, const float _y ) : x( _x ), y (_y ) { }
			Vector2d operator+( const Vector2d& other );
			Vector2d operator-( const Vector2d& other );
			Vector2d operator*( const Vector2d& other );
			Vector2d operator/( const Vector2d& other );
			void operator=( const Vector2d& other );
			void operator+=( const Vector2d& other );
			void operator-=( const Vector2d& other );
			void operator/=( const Vector2d& other );
			void operator+=( const float other );
			void operator-=( const float other );
			void operator*=( const float other );
			void operator/=( const float other );
		};

		struct Colour
		{
			char r;
			char g;
			char b;

			Colour() : r( 0 ), g( 0 ), b( 0 ) { }
			Colour( const char _r, const char _g, const char _b ) : r( _r ), g( _g ), b( _b ) { }
		};
	}
}