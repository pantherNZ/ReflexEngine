#pragma once

// Includes
#include "CommonIncludes.h"

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

			bool Initialise();
			void RenderBegin();
			void RenderEnd();

			ALLEGRO_DISPLAY_MODE GetMaxResolution() const;
			ALLEGRO_DISPLAY* GetDisplay() const;
			int GetWidth() const;
			int GetHeight() const;
			bool SetFullscreen( const bool fullscreen, const bool force = false );

		private:
			ALLEGRO_DISPLAY* m_display = nullptr;
			bool m_fullscreen = true;
		};
	}
}