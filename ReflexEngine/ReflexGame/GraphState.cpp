#include "GraphState.h"

#include <fstream>
#include <set>

namespace Reflex
{
	enum class ResourceID : unsigned short
	{
		ArialFontID,
		GraphNodeTextureID,
	};
}

GraphState::GraphState( Reflex::Core::StateManager& stateManager, Reflex::Core::Context context )
	: State( stateManager, context )
	, m_world( *context.window )
	, m_bounds( 0.0f, 0.0f, ( float )context.window->getSize().x, ( float )context.window->getSize().y )
{
	context.fontManager->LoadResource( Reflex::ResourceID::ArialFontID, "Data/Fonts/arial.ttf" );
	//context.textureManager->LoadResource( Reflex::ResourceID::GraphNodeTextureID, "Data/Textures/GraphNode.png" );

	ParseFile( "Data/VirtualStats.cpp" );

	m_world.CreateObject();
	// CreateObject(
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
	std::map< std::string, GraphNode* > hashMap;
	auto* baseNode = m_world.GetWorldGraphFromLayer( 0 );

	std::ifstream input( fileName );

	const std::string strVirtual = "VIRTUAL_STAT(";
	std::string strLine;
	int iCurrentNodeIndex = 0;

	bool bHaveCurrentNode = false;

	while( !input.eof() )
	{
		// Process each line
		std::getline( input, strLine );

		// Trim
		strLine = Reflex::Trim( strLine );

		// Check for a virtual stat line
		if( strLine.substr( 0, strVirtual.size() ) == strVirtual )
		{
			// Narrow down to find the stat name
			std::string strStatName = strLine.substr( strVirtual.size(), strLine.size() - strVirtual.size() );
			int uiCounter = Reflex::IndexOf( strLine, ',' );
			strStatName = strStatName.substr( 0, ( uiCounter == -1 ? strStatName.size() : uiCounter ) );
			strStatName = Reflex::Trim( strStatName );

			// Add the virtual stat to the list of initial nodes (if it doesn't already exist)
			if( hashMap.find( strStatName ) == hashMap.end())
			{
				iCurrentNodeIndex = baseNode->GetChildrenCount();
				auto pNewNode = std::make_unique< GraphNode >( GetContext().fontManager->GetResource( Reflex::ResourceID::ArialFontID ), strStatName );
				pNewNode->setPosition( m_bounds.left + Reflex::RandomFloat() * m_bounds.width, m_bounds.top + Reflex::RandomFloat() * m_bounds.height );
				hashMap.insert( std::make_pair( strStatName, pNewNode.get() ) );
				baseNode->AttachChild( std::move( pNewNode ) );
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
				auto strStatTrimmed = Reflex::Trim( strStat );

				if( strStatTrimmed.size() == 0 )
					continue;

				if( strStatTrimmed.back() == ')' )
				{
					bVirtualStatComplete = true;
					strStatTrimmed.pop_back();
					strStatTrimmed = Reflex::Trim( strStatTrimmed );
				}

				// Find a matching stat in our list
				const auto found = hashMap.find( strStatTrimmed );

				if( found != hashMap.end() )
				{
					// Add to connection list
					found->second->m_connections.push_back( static_cast< GraphNode* >( baseNode->GetChild( iCurrentNodeIndex ) ) );
				}
				else
				{
					// If we didn't find a matching stat, create a new one
					auto pNewNode = std::make_unique< GraphNode >( GetContext().fontManager->GetResource( Reflex::ResourceID::ArialFontID ), strStatTrimmed );
					pNewNode->setPosition( m_bounds.left + Reflex::RandomFloat() * m_bounds.width, m_bounds.top + Reflex::RandomFloat() * m_bounds.height );
					pNewNode->m_connections.push_back( static_cast< GraphNode* >( baseNode->GetChild( baseNode->GetChildrenCount() - 1 ) ) );
					hashMap.insert( std::make_pair( strStatTrimmed, pNewNode.get() ) );
					baseNode->AttachChild( std::move( pNewNode ) );
				}
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