#pragma once

#include "System.h"

namespace Reflex
{
	namespace Systems
	{
		class RenderSystem : public System
		{
		public:
			using System::System;

			void RegisterComponents() final;
			void Update( const sf::Time deltaTime ) final { }
			void Render( sf::RenderTarget& target, sf::RenderStates states ) const final;
			void OnSystemStartup() final {}
			void OnSystemShutdown() final { }
		};
	}
}