#pragma once

#include "Component.h"

namespace Reflex
{
	namespace Components
	{
		class TransformComponent : public Component, public sf::Transformable
		{
		public:
			using Component::Component;
		};
	}
}