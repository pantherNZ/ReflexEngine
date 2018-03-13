#include "GraphState.h"
#include "GraphNode.h"
#include "GraphRenderer.h"

#include "..\ReflexEngine\TransformComponent.h"
#include "..\ReflexEngine\SpriteComponent.h"
#include "..\ReflexEngine\RenderSystem.h"

#include <fstream>

namespace Reflex
{
	enum class ResourceID : unsigned short
	{
		ArialFontID,
		GraphNodeTextureID,
	};
}

GraphState::GraphState( StateManager& stateManager, Context context )
	: State( stateManager, context )
	, m_world( *context.window )
	, m_bounds( 0.0f, 0.0f, ( float )context.window->getSize().x, ( float )context.window->getSize().y )
	, m_ga( "Data/VirtualStats.cpp", m_bounds )
{
	context.fontManager->LoadResource( Reflex::ResourceID::ArialFontID, "Data/Fonts/arial.ttf" );
	context.textureManager->LoadResource( Reflex::ResourceID::GraphNodeTextureID, "Data/Textures/GraphNode.png" );

	m_gaInfo.setFont( context.fontManager->GetResource( Reflex::ResourceID::ArialFontID ) );
	m_gaInfo.setPosition( 5.0f, 50.0f );

	m_world.AddSystem< GraphRenderer >();
}

Reflex::Core::ObjectHandle GraphState::CreateGraphObject( const sf::Vector2f& position, const std::string& label )
{
	auto object = m_world.CreateObject();
	object->AddComponent< GraphNode >( sf::Color::Red, 10.0f, label, GetContext().fontManager->GetResource( Reflex::ResourceID::ArialFontID ) );

	auto transform = object->AddComponent< Reflex::Components::TransformComponent >();
	transform->setPosition( position );

	return object;
}

void GraphState::Render()
{
	m_world.Render();

	m_gaInfo.setString( sf::String( Reflex::ToString( 
		"Best GA Score: ", m_ga.GetBestGraph().score, 
		"\nAverage GA Score: ", m_ga.GetAverageScore() ) ) );
	
	GetContext().window->draw( m_gaInfo );
}

bool GraphState::Update( const sf::Time deltaTime )
{
	m_world.Update( deltaTime );

	m_gaUpdateTimer -= deltaTime.asSeconds();
	m_gaRenderTimer -= deltaTime.asSeconds();

	if( m_gaUpdateTimer <= 0.0f )
	{
		m_gaUpdateTimer += GA_UpdateIntervalMS / 1000.0f;
		m_ga.IteratePopulation();
	}

	if( m_gaRenderTimer <= 0.0f )
	{
		m_gaRenderTimer += GA_RenderIntervalMS / 1000.0f;
		UpdateGeneticAlgorithm();
	}

	return true;
}

bool GraphState::ProcessEvent( const sf::Event& event )
{
	if( event.type == sf::Event::KeyReleased )
	{
		if( event.key.code == sf::Keyboard::Space )
		{
			UpdateGeneticAlgorithm();
		}
	}

	return true;
}

void GraphState::UpdateGeneticAlgorithm()
{
	const auto graph = m_ga.GetBestGraph();

	std::vector< std::pair< Handle< GraphNode >, Handle< Reflex::Components::TransformComponent > > > nodes;

	m_world.DestroyAllObjects();

	// Copy / create objects
	for( auto& node : graph.nodes )
	{
		const auto obj = CreateGraphObject( node.position, node.label );
		nodes.emplace_back( obj->GetComponent< GraphNode >(), obj->GetComponent< Reflex::Components::TransformComponent >() );
	}

	// Copy / create connections
	for( auto connection : graph.connections )
		nodes[connection.from].first->m_connections.push_back( nodes[connection.to].second );

	m_world.GetSystem< GraphRenderer >()->RebuildVertexArray();
}