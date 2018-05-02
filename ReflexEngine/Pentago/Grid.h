#pragma once

#include "..\ReflexEngine\Engine.h"

using namespace Reflex::Core;

class Grid
{
public:
	Grid( World& world );

	void AddGridObject( const bool black, const sf::Vector2f& position );
	void AddGridObject( const bool black, const unsigned x, const unsigned y );
	sf::Vector2f GetGridPosition( const unsigned x, const unsigned y ) const;
	sf::Vector2u GetGridIndex( const sf::Vector2f& position ) const;
	void RemoveGridObject( const unsigned x, const unsigned y );
	ObjectHandle GetGridObject( const sf::Vector2f& position ) const;

protected:
	World& m_world;
	std::array< std::pair< std::vector< std::vector< ObjectHandle > >, ObjectHandle >, 4 > m_grid;
	Reflex::AABB m_bounds;

	const float m_centreOffset = 10.0f;
	float m_circleDiameter = 0.0f;
	float m_circleOffset = 0.0f;
	sf::Vector2f m_topLeft;
};