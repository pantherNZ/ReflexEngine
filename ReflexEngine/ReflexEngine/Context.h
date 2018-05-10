#pragma once

#include "Precompiled.h"
#include "ResourceManager.h"

namespace Reflex
{
	namespace Core
	{
		struct Context
		{
			Context( sf::RenderWindow& _window, TextureManager& _textureManager, FontManager& _fontManager )
				: window( &_window )
				, textureManager( &_textureManager )
				, fontManager( &_fontManager )
			{
			}

			sf::RenderWindow* window;
			TextureManager* textureManager;
			FontManager* fontManager;
		};
	}
}