#pragma once

#include "..\ReflexEngine\Engine.h"
#include "..\ReflexEngine\QuadTree.h"

using namespace Reflex::Core;

class GameBoard
{
public:
	GameBoard( World& world, const bool playerIsWhite );

	//void PlaceMarble( const bool black, const sf::Vector2f& position );
	//void PlaceMarble( const bool black, const unsigned x, const unsigned y );
	//void PlaceMarble( const bool black, const sf::Vector2u& index );
	//ObjectHandle GetMarble( const sf::Vector2f& position ) const;
	//ObjectHandle GetMarble( const unsigned x, const unsigned y ) const;
	//ObjectHandle GetMarble( const sf::Vector2u& index ) const;

public:
	World& m_world;
	Reflex::AABB m_bounds;
	float m_marbleSize = 0.0f;

	ObjectHandle m_selectedMarble;
};