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
			Transform( const sf::Vector2f& position = sf::Vector2f(), const float rotation = 0.0f, const sf::Vector2f& scale = sf::Vector2f( 1.0f, 1.0f ) );
			Transform( const Transform& other );

			void OnConstructionComplete() final;

			void setPosition( float x, float y );
			void setPosition( const sf::Vector2f& position );

			void move( float offsetX, float offsetY );
			void move( const sf::Vector2f& offset );

			virtual void SetOwningObject( const ObjectHandle& owner ) override;
		};
	}
}