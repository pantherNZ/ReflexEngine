#pragma once

#include "..\ReflexEngine\Component.h"
#include "..\ReflexEngine\TransformComponent.h"

using namespace Reflex::Core;

class GraphNode : public Reflex::Components::Component
{
public:
	GraphNode( const sf::Color& colour, const float size, const std::string& label, const sf::Font& font )
		: m_shape( sf::Vector2f( size, size ) )
		, m_label( label, font )
	{
		m_shape.setFillColor( colour );
		Reflex::CenterOrigin( m_shape );
		Reflex::CenterOrigin( m_label );
	}

	sf::RectangleShape m_shape;
	sf::Text m_label;
	std::vector< Handle< Reflex::Components::Transform > > m_connections;
	std::vector< unsigned > m_vertexArrayIndices;
};