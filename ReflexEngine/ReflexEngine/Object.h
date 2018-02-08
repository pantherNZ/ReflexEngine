#pragma once

#include "Common.h"
#include "SceneNode.h"

namespace Reflex
{
	namespace Core
	{
		class Object : public SceneNode
		{
		public:
			void SetVelocity( sf::Vector2f velocity );
			void SetVelocity( float vx, float vy );
			sf::Vector2f GetVelocity() const;

		protected:
			virtual void UpdateCurrent( const sf::Time deltaTime ) override;

		private:
			sf::Vector2f m_velocity;
		};
	}
}