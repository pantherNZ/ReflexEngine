#include "GraphRenderer.h"
#include "GraphNode.h"

#include "..\ReflexEngine\World.h"
#include "..\ReflexEngine\TransformComponent.h"

using namespace Reflex::Components;

void GraphRenderer::RegisterComponents()
{
	RequiresComponent( GraphNode );
	RequiresComponent( TransformComponent );
}

void GraphRenderer::Update( const sf::Time deltaTime )
{
	
}

void GraphRenderer::Render( sf::RenderTarget& target, sf::RenderStates states ) const
{
	sf::RenderStates copied_states( states );

	target.draw( m_connections, states );

	for( auto& component : m_components )
	{
		auto transform = GetSystemComponent< TransformComponent >( component );
		copied_states.transform = states.transform * transform->getTransform();

		auto node = GetSystemComponent< GraphNode >( component );
		target.draw( node->m_shape, copied_states );
		//target.draw( node->m_label, copied_states );
	}
}

void GraphRenderer::RebuildVertexArray()
{
	m_connections = sf::VertexArray( sf::PrimitiveType::Lines );

	// Create vertex array for graph nodes
	for( auto& component : m_components )
	{
		auto transform = GetSystemComponent< TransformComponent >( component );
		auto node = GetSystemComponent< GraphNode >( component );

		for( unsigned i = 0U; i < node->m_connections.size(); ++i )
		{
			node->m_vertexArrayIndices.push_back( m_connections.getVertexCount() );
			m_connections.append( sf::Vertex( transform->getPosition(), sf::Color::White ) );

			node->m_connections[i]->GetObject()->GetComponent< GraphNode >()->m_vertexArrayIndices.push_back( m_connections.getVertexCount() );
			m_connections.append( sf::Vertex( node->m_connections[i]->getPosition(), sf::Color::White ) );
		}
	}
}