#include "GraphState.h"
#include "GraphNode.h"
#include "GeneticAlgorithm.h"

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
{
	context.fontManager->LoadResource( Reflex::ResourceID::ArialFontID, "Data/Fonts/arial.ttf" );
	context.textureManager->LoadResource( Reflex::ResourceID::GraphNodeTextureID, "Data/Textures/GraphNode.png" );

	ParseFile( "Data/VirtualStats.cpp" );

	m_world.AddSystem< GeneticAlgorithm >();
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
}

bool GraphState::Update( const sf::Time deltaTime )
{
	m_world.Update( deltaTime );

	return true;
}

bool GraphState::ProcessEvent( const sf::Event& event )
{
	return true;
}

void GraphState::ParseFile( const std::string& fileName )
{
	std::map< std::string, ObjectHandle > hashMap;

	std::ifstream input( fileName );

	const std::string strVirtual = "VIRTUAL_STAT(";
	std::string strLine;
	ObjectHandle lastAdded;

	bool bHaveCurrentNode = false;

	while( !input.eof() )
	{
		// Process each line
		std::getline( input, strLine );

		// Trim
		Reflex::Trim( strLine );

		// Check for a virtual stat line
		if( strLine.substr( 0, strVirtual.size() ) == strVirtual )
		{
			// Narrow down to find the stat name
			std::string strStatName = strLine.substr( strVirtual.size(), strLine.size() - strVirtual.size() );
			int uiCounter = strStatName.find_first_of( ',' );
			strStatName = strStatName.substr( 0, ( uiCounter == std::string::npos ? strStatName.size() : uiCounter ) );
			Reflex::Trim( strStatName );

			// Add the virtual stat to the list of initial nodes (if it doesn't already exist)
			if( hashMap.find( strStatName ) == hashMap.end())
			{
				auto object = CreateGraphObject( sf::Vector2f( m_bounds.left + Reflex::RandomFloat() * m_bounds.width, m_bounds.top + Reflex::RandomFloat() * m_bounds.height ), strStatName );
				hashMap.insert( std::make_pair( strStatName, object ) );
				lastAdded = object;
				bHaveCurrentNode = true;
			}
		}
		else if( bHaveCurrentNode )
		{
			// Split the line incase there is multiple stats on a line
			std::vector< std::string > strStatsArray = Reflex::Split( strLine, ',' );
			bool bVirtualStatComplete = false;

			// For eachstat
			for( auto strStat : strStatsArray )
			{
				// Trim space
				auto strStatTrimmed( strStat );
				Reflex::Trim( strStatTrimmed );

				if( strStatTrimmed.size() == 0 )
					continue;

				if( strStatTrimmed.back() == ')' )
				{
					bVirtualStatComplete = true;
					strStatTrimmed.pop_back();
					Reflex::Trim( strStatTrimmed );
				}

				// Find a matching stat in our list
				auto found = hashMap.find( strStatTrimmed );

				if( found == hashMap.end() )
				{
					// If we didn't find a matching stat, create a new one
					auto object = CreateGraphObject( sf::Vector2f( m_bounds.left + Reflex::RandomFloat() * m_bounds.width, m_bounds.top + Reflex::RandomFloat() * m_bounds.height ), strStatTrimmed );
					found = hashMap.insert( std::make_pair( strStatTrimmed, object ) ).first;
				}

				// Add to connection list
				auto graphNode = found->second->GetComponent< GraphNode >();
				graphNode->m_connections.push_back( lastAdded->GetComponent< Reflex::Components::TransformComponent >() );
			}

			if( bVirtualStatComplete )
				bHaveCurrentNode = false;
		}
	}

	input.close();
}

void GraphState::GenerateGraphNodes()
{

}