#pragma once

#include "Component.h"
#include "SceneNode.h"
#include "MovementSystem.h"

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
			friend class Reflex::Systems::MovementSystem;

			Transform( const sf::Vector2f& position = sf::Vector2f(), const float rotation = 0.0f, const sf::Vector2f& scale = sf::Vector2f( 1.0f, 1.0f ) );

			void OnConstructionComplete() final;

			void setPosition( float x, float y );
			void setPosition( const sf::Vector2f& position );

			void move( float offsetX, float offsetY );
			void move( const sf::Vector2f& offset );

			void RotateForDuration( const float degrees, const float durationSec );
			void RotateForDuration( const float degrees, const float durationSec, std::function< void( const TransformHandle& ) > finishedRotationCallback );
			void StopRotation();

			virtual void SetOwningObject( const ObjectHandle& owner ) override;

		protected:
			float m_rotateDegreesPerSec = 0.0f;
			float m_rotateDurationSec = 0.0f;
			std::function< void( const TransformHandle& ) > m_finishedRotationCallback;
		};
	}
}