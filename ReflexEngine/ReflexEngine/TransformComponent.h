#pragma once

#include "Component.h"

namespace Reflex
{
	namespace Components
	{
		class TransformComponent : public Component, public sf::Transformable
		{
		public:
			TransformComponent( ObjectHandle object, BaseHandle handle, 
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