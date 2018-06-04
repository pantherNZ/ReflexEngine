
#include "SFMLObjectComponent.h"

namespace Reflex
{
	namespace Components
	{
		SFMLObject::SFMLObject( const sf::CircleShape& shape )
			: m_type( Circle )
			, m_objectData( shape )
		{
			Reflex::CenterOrigin( m_objectData.circleShape );
		}

		SFMLObject::SFMLObject( const sf::ConvexShape& shape )
			: m_type( Convex )
			, m_objectData( shape )
		{
			Reflex::CenterOrigin( m_objectData.convexShape );
		}

		SFMLObject::SFMLObject( const sf::RectangleShape& shape )
			: m_type( Rectangle )
			, m_objectData( shape )
		{
			Reflex::CenterOrigin( m_objectData.rectShape );
		}

		SFMLObject::SFMLObject( const sf::Sprite& sprite )
			: m_type( Sprite )
			, m_objectData( sprite )
		{
			Reflex::CenterOrigin( m_objectData.sprite );
		}

		SFMLObject::SFMLObject( const SFMLObject& other )
			: Component( other )
			, m_type( other.m_type )
		{
			memcpy( &this->m_objectData, &other.m_objectData, sizeof( ObjectType ) );
		}

		sf::CircleShape& SFMLObject::GetCircleShape()
		{ 
			return m_objectData.circleShape;
		}

		const sf::CircleShape& SFMLObject::GetCircleShape() const
		{
			return m_objectData.circleShape;
		}

		sf::RectangleShape& SFMLObject::GetRectangleShape()
		{ 
			return m_objectData.rectShape;
		}

		const sf::RectangleShape& SFMLObject::GetRectangleShape() const
		{
			return m_objectData.rectShape;
		}

		sf::ConvexShape& SFMLObject::GetConvexShape()
		{ 
			return m_objectData.convexShape;
		}

		const sf::ConvexShape& SFMLObject::GetConvexShape() const
		{
			return m_objectData.convexShape;
		}

		sf::Sprite& SFMLObject::GetSprite()
		{ 
			return m_objectData.sprite;
		}

		const sf::Sprite& SFMLObject::GetSprite() const
		{
			return m_objectData.sprite;
		}

		const Reflex::Components::SFMLObjectType SFMLObject::GetType() const
		{
			return m_type;
		}
	}
}