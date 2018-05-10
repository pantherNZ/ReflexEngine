
#include "SFMLObjectComponent.h"

namespace Reflex
{
	namespace Components
	{
		SFMLObject::SFMLObject( const ObjectHandle& object, const BaseHandle& componentHandle, const sf::CircleShape& shape )
			: Component( object, componentHandle )
			, m_type( Rectangle )
			, m_object( shape )
		{
			Reflex::CenterOrigin( m_object.circleShape );
		}

		SFMLObject::SFMLObject( const ObjectHandle& object, const BaseHandle& componentHandle, const sf::ConvexShape& shape )
			: Component( object, componentHandle )
			, m_type( Rectangle )
			, m_object( shape )
		{
			Reflex::CenterOrigin( m_object.convexShape );
		}

		SFMLObject::SFMLObject( const ObjectHandle& object, const BaseHandle& componentHandle, const sf::RectangleShape& shape )
			: Component( object, componentHandle )
			, m_type( Rectangle )
			, m_object( shape )
		{
			Reflex::CenterOrigin( m_object.rectShape );
		}

		SFMLObject::SFMLObject( const ObjectHandle& object, const BaseHandle& componentHandle, const sf::Sprite& sprite )
			: Component( object, componentHandle )
			, m_type( Sprite )
			, m_object( sprite )
		{
			Reflex::CenterOrigin( m_object.sprite );
		}

		sf::CircleShape& SFMLObject::GetCircleShape() 
		{ 
			return m_object.circleShape;
		}

		sf::RectangleShape& SFMLObject::GetRectangleShape() 
		{ 
			return m_object.rectShape;
		}

		sf::ConvexShape& SFMLObject::GetConvexShape() 
		{ 
			return m_object.convexShape;
		}

		sf::Sprite& SFMLObject::GetSprite() 
		{ 
			return m_object.sprite; 
		}

		const Reflex::Components::SFMLObjectType SFMLObject::GetType() const
		{
			return m_type;
		}
	}
}