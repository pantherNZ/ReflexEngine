#include "GraphNode.h"

GraphNode::GraphNode( Reflex::Core::Object& object, const sf::Font& font, const sf::String text )
	: Component( object )
	, m_label( text, font )
	, m_image( 5 )
{
	Reflex::CenterOrigin( m_image );
	Reflex::CenterOrigin( m_label );

	m_image.setFillColor( sf::Color::Red );
}

/*
void GraphNode::DrawCurrent( sf::RenderTarget& target, sf::RenderStates states ) const
{
	const auto thisPosition = GetWorldPosition();

	sf::Vertex line[2] =
	{
		sf::Vertex( thisPosition ),
		sf::Vertex( thisPosition )
	};

	for( auto& connection : m_connections )
	{
		line[1] = connection->GetWorldPosition();
		target.draw( line, 2, sf::Lines );
	}
	
	target.draw( m_image, states );
	//target.draw( m_label );
}*/
