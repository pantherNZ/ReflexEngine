#pragma once

// Includes
#include "Common.h"

#include <SFML/Graphics.hpp>

// Window class
namespace Reflex
{
	namespace Core
	{
		class Window
		{
		public:
			Window();
			~Window();

			void RenderBegin();
			void RenderEnd();

		private:
		};
	}
}