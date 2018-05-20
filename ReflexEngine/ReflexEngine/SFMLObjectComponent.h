#pragma once

#include "Component.h"

namespace Reflex
{
	namespace Components
	{
		class SFMLObject;
	}

	namespace Core
	{
		typedef Handle< class Reflex::Components::SFMLObject > SFMLObjectHandle;
	}

	namespace Components
	{
		enum SFMLObjectType
		{
			Invalid,
			Circle,
			Rectangle,
			Convex,
			Sprite,
		};

		// Class definition
		class SFMLObject : public Component
		{
		public:
			SFMLObject( const sf::CircleShape& shape );
			SFMLObject( const sf::ConvexShape& shape );
			SFMLObject( const sf::RectangleShape& shape );
			SFMLObject( const sf::Sprite& sprite );
			SFMLObject( const SFMLObject& other );
			~SFMLObject();

			// Get functions
			sf::CircleShape& GetCircleShape();
			const sf::CircleShape& GetCircleShape() const;

			sf::RectangleShape& GetRectangleShape();
			const sf::RectangleShape& GetRectangleShape() const;
			
			sf::ConvexShape& GetConvexShape();
			const sf::ConvexShape& GetConvexShape() const;

			sf::Sprite& GetSprite();
			const sf::Sprite& GetSprite() const;

			const SFMLObjectType GetType() const;

		private:
			union ObjectType
			{
				sf::CircleShape circleShape;			// 292 bytes
				sf::RectangleShape rectShape;			// 292 bytes
				sf::ConvexShape convexShape;			// 300 bytes
				sf::Sprite sprite;						// 272 bytes

				ObjectType() {}
				ObjectType( const sf::CircleShape& shape ) : circleShape( shape ) {}
				ObjectType( const sf::RectangleShape& shape ) : rectShape( shape ) {}
				ObjectType( const sf::ConvexShape& shape ) : convexShape( shape ) {}
				ObjectType( const sf::Sprite& sprite ) : sprite( sprite ) {}
				~ObjectType() {}
			};

			ObjectType m_objectData;
			SFMLObjectType m_type = Invalid;
		};
	}
}