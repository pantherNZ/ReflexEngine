#pragma once

#include "System.h"

namespace Reflex
{
	namespace Systems
	{
		class InteractableSystem : public System
		{
		public:
			using System::System;

			void RegisterComponents() final;
			void Update( const float deltaTime ) final;
			void ProcessEvent( const sf::Event& event ) final;
			void OnSystemStartup() final {}
			void OnSystemShutdown() final {}

		protected:
			bool m_mousePressed = false;
			bool m_mouseReleased = false;
		};
	}
}