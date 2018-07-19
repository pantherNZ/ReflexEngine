#pragma once

#include "Precompiled.h"
#include "ResourceManager.h"
#include "HandleManager.h"

namespace Reflex
{
	namespace Core
	{
		struct Context
		{
			Context( HandleManager& _handleManager, sf::RenderWindow& _window, TextureManager& _textureManager, FontManager& _fontManager )
				: handleManager( &_handleManager )
				, window( &_window )
				, textureManager( &_textureManager )
				, fontManager( &_fontManager )
			{
			}

			HandleManager* handleManager;
			sf::RenderWindow* window;
			TextureManager* textureManager;
			FontManager* fontManager;
		};
	}
}