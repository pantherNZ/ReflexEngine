#pragma once

#include "..\ReflexEngine\Engine.h"
#include "..\ReflexEngine\QuadTree.h"

using namespace Reflex::Core;

class GameBoard
{
public:
	GameBoard( World& world, const bool playerIsWhite );

public:
	World& m_world;
	sf::FloatRect m_boardBounds;
	float m_marbleSize = 0.0f;

	ObjectHandle m_selectedMarble;
};