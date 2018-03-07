#pragma once

#include "Component.h"

namespace Reflex
{
	namespace Components
	{
		class SpriteComponent : public Component, public sf::Sprite
		{
		public:
			SpriteComponent( ObjectHandle object, BaseHandle handle ) : Component( object, handle ) { Reflex::CenterOrigin( *this ); }
		};
	}
}