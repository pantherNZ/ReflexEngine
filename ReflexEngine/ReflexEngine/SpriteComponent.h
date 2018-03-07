#pragma once

#include "Component.h"

namespace Reflex
{
	namespace Components
	{
		class SpriteComponent : public Component, public sf::Sprite
		{
		public:
			SpriteComponent( Object& object, BaseHandle handle ) : Component( object, handle ) { Reflex::CenterOrigin( *this ); }

		private:
			//void draw( sf::RenderTarget& target, sf::RenderStates states ) const final { target.draw( *this ); }
		};
	}
}