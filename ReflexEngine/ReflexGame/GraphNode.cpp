#include "GraphNode.h"

GraphNode::GraphNode()// const Reflex::Core::TextureManager& textureManager )
{
	sf::FloatRect bounds = m_sprite.getLocalBounds();
	m_sprite.setOrigin( bounds.width / 2.f, bounds.height / 2.f );
}

void GraphNode::DrawCurrent( sf::RenderTarget& target, sf::RenderStates states ) const
{
	target.draw( m_sprite );
}
