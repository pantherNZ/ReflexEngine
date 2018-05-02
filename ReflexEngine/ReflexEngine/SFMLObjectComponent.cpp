
#include "SFMLObjectComponent.h"

namespace Reflex
{
	namespace Components
	{
		SFMLObject::SFMLObject( const ObjectHandle& object, const BaseHandle& handle, const sf::CircleShape& shape )
			: Component( object, handle )
			, m_type( Rectangle )
			, m_object( shape )
		{
			Reflex::CenterOrigin( m_object.circleShape );
		}

		SFMLObject::SFMLObject( const ObjectHandle& object, const BaseHandle& handle, const sf::ConvexShape& shape )
			: Component( object, handle )
			, m_type( Rectangle )
			, m_object( shape )
		{
			Reflex::CenterOrigin( m_object.convexShape );
		}

		SFMLObject::SFMLObject( const ObjectHandle& object, const BaseHandle& handle, const sf::RectangleShape& shape )
			: Component( object, handle )
			, m_type( Rectangle )
			, m_object( shape )
		{
			Reflex::CenterOrigin( m_object.rectShape );
		}

		SFMLObject::SFMLObject( const ObjectHandle& object, const BaseHandle& handle, const sf::Sprite& sprite )
			: Component( object, handle )
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