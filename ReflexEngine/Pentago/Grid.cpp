#include "Grid.h"

#include "..\ReflexEngine\SFMLObjectComponent.h"

Grid::Grid( World& world )
	: m_world( world )
{
	m_bounds = Reflex::ToAABB( m_world.GetBounds() );
	m_bounds.halfSize = sf::Vector2f( m_bounds.halfSize.y, m_bounds.halfSize.y ) * 0.6f + sf::Vector2f( m_centreOffset, m_centreOffset );
	m_grid[0].second = m_world.CreateObject( m_bounds.centre + sf::Vector2f( -m_bounds.halfSize.x, -m_bounds.halfSize.y ) / 2.0f );
	m_grid[1].second = m_world.CreateObject( m_bounds.centre + sf::Vector2f( m_bounds.halfSize.x, -m_bounds.halfSize.y ) / 2.0f );
	m_grid[2].second = m_world.CreateObject( m_bounds.centre + sf::Vector2f( -m_bounds.halfSize.x, m_bounds.halfSize.y ) / 2.0f );
	m_grid[3].second = m_world.CreateObject( m_bounds.centre + sf::Vector2f( m_bounds.halfSize.x, m_bounds.halfSize.y ) / 2.0f );

	m_bounds.halfSize -= sf::Vector2f( m_centreOffset, m_centreOffset );

	for( auto& base : m_grid )
	{
		auto component = base.second->AddComponent< Reflex::Components::SFMLObject >( sf::RectangleShape( m_bounds.halfSize ) );
		component->GetRectangleShape().setFillColor( sf::Color::Green );
		base.first.resize( 6 );

		for( auto& y : base.first )
			y.resize( 6 );
	}

	m_circleDiameter = m_bounds.halfSize.x / 5.0f;
	m_circleOffset = m_bounds.halfSize.x / 3.0f;
	m_topLeft = m_bounds.centre - m_bounds.halfSize + sf::Vector2f( ( m_circleDiameter - m_centreOffset ) / 2.0f, ( m_circleDiameter - m_centreOffset ) / 2.0f );

	for( unsigned y = 0U; y < 6; ++y )
	{
		for( unsigned x = 0U; x < 6; ++x )
		{
			AddGridObject( false, x, y );
		}
	}
}

void Grid::AddGridObject( const bool black, const sf::Vector2f& position )
{
	const auto bottomRight = GetGridPosition( 2, 2 );

	if( position.x >= m_topLeft.x && position.y >= m_topLeft.y && 
		position.x <= bottomRight.x && position.y <= bottomRight.y )
	{
		const auto index = GetGridIndex( position );
		AddGridObject( black, index.x, index.y );
	}
}

void Grid::AddGridObject( const bool black, const unsigned x, const unsigned y )
{
	const auto baseIndex = x / 3 + y / 3;

	if( baseIndex < 4 && y < m_grid[baseIndex].first.size() && x < m_grid[baseIndex].first[y].size() )
	{	
		m_grid[baseIndex].first[y][x] = m_world.CreateObject( GetGridPosition( x, y ) );
		auto component = m_grid[baseIndex].first[y][x]->AddComponent< Reflex::Components::SFMLObject >( sf::CircleShape( m_circleDiameter / 2.0f ) );
		component->GetRectangleShape().setFillColor( black ? sf::Color::Black : sf::Color::White );
	}
}

sf::Vector2f Grid::GetGridPosition( const unsigned x, const unsigned y ) const
{
	const auto offsetX = x * m_circleOffset + ( x > 2 ? m_centreOffset : 0.0f );
	const auto offsetY = y * m_circleOffset + ( y > 2 ? m_centreOffset : 0.0f );
	return m_topLeft + sf::Vector2f( offsetX, offsetY );
}

sf::Vector2u Grid::GetGridIndex( const sf::Vector2f& position ) const
{
	const auto correctPosition = position - m_topLeft;
	const auto x = ( correctPosition.x + Reflex::Sign( correctPosition.x ) * m_centreOffset / 2.0f ) / m_circleOffset;
	const auto y = ( correctPosition.y + Reflex::Sign( correctPosition.y ) * m_centreOffset / 2.0f ) / m_circleOffset;
	return sf::Vector2u( ( unsigned )x, ( unsigned ) y );
}

void Grid::RemoveGridObject( const unsigned x, const unsigned y )
{

}

ObjectHandle Grid::GetGridObject( const sf::Vector2f& position ) const
{

	return ObjectHandle();
}