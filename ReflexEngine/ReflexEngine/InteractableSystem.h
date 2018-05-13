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
			void OnSystemStartup() final {}
			void OnSystemShutdown() final {}
		};
	}
}