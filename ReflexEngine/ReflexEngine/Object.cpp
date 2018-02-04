#include "Object.h"

namespace Reflex
{
	namespace Core
	{
		void Object::SetVelocity( sf::Vector2f velocity )
		{
			m_velocity = velocity;
		}

		void Object::SetVelocity( float vx, float vy )
		{
			m_velocity = sf::Vector2f( vx, vy );
		}

		sf::Vector2f Object::GetVelocity() const
		{
			return m_velocity;
		}

		void Object::UpdateCurrent( const sf::Time deltaTime )
		{
			move( m_velocity * deltaTime.asSeconds() );
		}
	}
}