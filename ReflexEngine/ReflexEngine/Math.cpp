#include "Math.h"

namespace Reflex
{
	namespace Core
	{
		Vector2d Vector2d::operator+( const Vector2d& other )
		{
			return Vector2d( x + other.x, y + other.y );
		}

		Vector2d Vector2d::operator-( const Vector2d& other )
		{
			return Vector2d( x - other.x, y - other.y );
		}

		Vector2d Vector2d::operator*( const Vector2d& other )
		{
			return Vector2d( x * other.x, y * other.y );
		}

		Vector2d Vector2d::operator/( const Vector2d& other )
		{
			return Vector2d( x / other.x, y / other.y );
		}

		void Vector2d::operator=( const Vector2d& other )
		{
			x = other.x;
			y = other.y;
		}

		void Vector2d::operator+=( const Vector2d& other )
		{
			x += other.x;
			y += other.y;
		}
		void Vector2d::operator-=( const Vector2d& other )
		{
			x -= other.x;
			y -= other.y;
		}

		void Vector2d::operator/=( const Vector2d& other )
		{
			x /= other.x;
			y /= other.y;
		}

		void Vector2d::operator+=( const float other )
		{
			x -= other;
			y -= other;
		}

		void Vector2d::operator-=( const float other )
		{
			x -= other;
			y -= other;
		}

		void Vector2d::operator*=( const float other )
		{
			x *= other;
			y *= other;
		}

		void Vector2d::operator/=( const float other )
		{
			x /= other;
			y /= other;
		}
	}
}