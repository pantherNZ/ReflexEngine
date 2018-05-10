#pragma once

#include "Component.h"
#include "SceneNode.h"

namespace Reflex
{
	namespace Components
	{
		class Transform;
	}

	namespace Core
	{
		typedef Handle< class Reflex::Components::Transform > TransformHandle;
	}

	namespace Components
	{
		class Transform : public Component, public Reflex::Core::SceneNode
		{
		public:
			Transform( const ObjectHandle& object, BaseHandle componentHandle,
				const sf::Vector2f& position = sf::Vector2f(), 
				const float rotation = 0.0f, 
				const sf::Vector2f& scale = sf::Vector2f( 1.0f, 1.0f ) );

			void setPosition( float x, float y );
			void setPosition( const sf::Vector2f& position );

			void move( float offsetX, float offsetY );
			void move( const sf::Vector2f& offset );
		};
	}
}