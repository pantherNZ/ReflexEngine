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
			Circle,
			Rectangle,
			Convex,
			Sprite,
		};

		// Class definition
		class SFMLObject : public Component
		{
		public:
			// Initialise by circle shape
			SFMLObject( const ObjectHandle& object, const BaseHandle& handle, const sf::CircleShape& shape );

			// Initialise by convex shape
			SFMLObject( const ObjectHandle& object, const BaseHandle& handle, const sf::ConvexShape& shape );

			// Initialise by rectangle shape
			SFMLObject( const ObjectHandle& object, const BaseHandle& handle, const sf::RectangleShape& shape );

			// Initialise by sprite
			SFMLObject( const ObjectHandle& object, const BaseHandle& handle, const sf::Sprite& sprite );

			// Get functions
			sf::CircleShape& GetCircleShape();
			sf::RectangleShape& GetRectangleShape();
			sf::ConvexShape& GetConvexShape();
			sf::Sprite& GetSprite();
			const SFMLObjectType GetType() const;

		private:
			union ObjectType
			{
				sf::CircleShape circleShape;			// 292 bytes
				sf::RectangleShape rectShape;			// 292 bytes
				sf::ConvexShape convexShape;			// 300 bytes
				sf::Sprite sprite;						// 272 bytes

				ObjectType( sf::CircleShape shape ) : circleShape( shape ) { }
				ObjectType( sf::RectangleShape shape ) : rectShape( shape ) { }
				ObjectType( sf::ConvexShape shape ) : convexShape( shape ) { }
				ObjectType( sf::Sprite sprite ) : sprite( sprite ) { }
				~ObjectType() { }
			};

			ObjectType m_object;
			SFMLObjectType m_type;
		};
	}
}