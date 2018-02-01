#pragma once

// Includes
#include "CommonIncludes.h"

// Forward declarations
namespace Reflex
{
	namespace Core
	{
		class Window;
		class Sprite;
	}
}

// Engine class
namespace Reflex
{
	namespace Core
	{
		class Engine
		{
		public:
			Engine();
			~Engine();

			bool Initialise();
			void Run();

		private:
			std::unique_ptr< Window > m_window = nullptr;
			ALLEGRO_TIMER* m_timer = nullptr;
			ALLEGRO_EVENT_QUEUE* m_event_queue = nullptr;

			std::unique_ptr< Sprite > m_sprite = nullptr;
			Vector2d velocity;
		};
	}
}