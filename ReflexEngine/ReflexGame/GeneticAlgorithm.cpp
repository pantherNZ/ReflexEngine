#include "GeneticAlgorithm.h"

#include <map>
#include <fstream>

#include "..\ReflexEngine\Utility.h"

GeneticAlgorithm::GeneticAlgorithm( const std::string& fileName, const sf::FloatRect& bounds )
	: m_bounds( bounds )
{
	m_population.reserve( GA_Population );
	ParseFile( fileName );

	for( int i = 0; i < GA_Population - 1; ++i )
	{
		m_population.emplace_back( m_population.front() );

		for( auto& node : m_population.back().nodes )
			node.position = sf::Vector2f( m_bounds.left + Reflex::RandomFloat() * m_bounds.width, m_bounds.top + Reflex::RandomFloat() * m_bounds.height );
	}
}

const GeneticAlgorithm::Graph& GeneticAlgorithm::GetBestGraph()
{
	return m_population.front();
}

const float GeneticAlgorithm::GetAverageScore()
{
	float average = 0.0f;

	for( auto& iter : m_population )
		average += iter.score;
	return average / m_population.size();
}

void GeneticAlgorithm::ParseFile( const std::string& fileName )
{
	m_population.emplace_back();
	std::map< std::string, unsigned > hashMap;

	std::ifstream input( fileName );

	const std::string strVirtual = "VIRTUAL_STAT(";
	std::string strLine;
	unsigned lastAdded;

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
			if( hashMap.find( strStatName ) == hashMap.end() )
			{
				const auto position = sf::Vector2f( m_bounds.left + Reflex::RandomFloat() * m_bounds.width, m_bounds.top + Reflex::RandomFloat() * m_bounds.height );
				m_population.back().nodes.emplace_back( Node( position, strStatName ) );
				lastAdded = m_population.back().nodes.size() - 1;
				hashMap.insert( std::make_pair( strStatName, lastAdded ) );
				bHaveCurrentNode = true;
			}
		}
		else if( bHaveCurrentNode )
		{
			// Split the line incase there is multiple stats on a line
			std::vector< std::string > strStatsArray = Reflex::Split( strLine, ',' );
			bool bVirtualStatComplete = false;

			// For each stat
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
					const auto position = sf::Vector2f( m_bounds.left + Reflex::RandomFloat() * m_bounds.width, m_bounds.top + Reflex::RandomFloat() * m_bounds.height );
					m_population.back().nodes.emplace_back( Node( position, strStatTrimmed ) );
					const auto index = m_population.back().nodes.size() - 1;
					hashMap.insert( std::make_pair( strStatTrimmed, index ) );
					m_population.back().connections.push_back( Connection( index, lastAdded ) );
					m_population.back().nodes.back().connections.insert( lastAdded );
				}
				else
				{
					// Add to connection list
					m_population.back().connections.push_back( Connection( found->second, lastAdded ) );
					m_population.back().nodes[found->second].connections.insert( lastAdded );
				}
			}

			if( bVirtualStatComplete )
				bHaveCurrentNode = false;
		}
	}

	input.close();
}

/* 
Initialization
Evaluation
Selection
Crossover
Mutation
*/
void GeneticAlgorithm::IteratePopulation()
{
	// Score the graph using heuristics
	for( auto& graph : m_population )
		ScoreGraph( graph );

	// Sort by score
	std::sort( m_population.begin(), m_population.end(), [&]( const Graph& graphA, const Graph& graphB )
	{
		return graphA.score < graphB.score;
	} );

	// Replace the weakest with new children from the strongest
	for( int i = 0; i < GA_CrossOverCount; ++i )
	{
		CrossOver( m_population[i * 2], m_population[i * 2 + 1], m_population.size() - 1 - i * 2 );
	}

	for( auto& graph : m_population )
		if( Reflex::RandomInt( 100 ) <= GA_MutationChance )
			Mutate( graph );
}

void GeneticAlgorithm::ScoreGraph( Graph& graph )
{
	graph.score = 0.0f;

	/* Minimum Node Distance Sum: The distance of each node from its nearest neighbour 
	is measured, and the distances are added up.The bigger the sum the more evenly the 
	nodes are usually distributed over the drawing area. */
	for( auto& node : graph.nodes )
	{
		float smallestDist = std::numeric_limits< float >::max();
		Node* closestNode = nullptr;

		for( auto& otherNode : graph.nodes )
		{
			if( &node != &otherNode )
			{
				const float distance = Reflex::GetDistanceSq( node.position, otherNode.position );

				if( distance < smallestDist )
				{
					smallestDist = distance;
					closestNode = &otherNode;
				}
			}
		}

		if( closestNode )
			graph.score -= smallestDist / 100.0f;
	}

	/* Edge Crossings: The total number of edge crossings are added up and
	counted towards the total score negatively scaled by a weighing X */
	int intersections = 0;

	for( auto& connection : graph.connections )
	{
		for( auto& otherConnection : graph.connections )
		{
			if( &connection != &otherConnection )
			{
				if( Reflex::IntersectLineLine( 
					graph.nodes[connection.from].position,
					graph.nodes[connection.to].position,
					graph.nodes[otherConnection.from].position,
					graph.nodes[otherConnection.to].position ) )
				{
					intersections++;
				}
			}
		}

		/* Optimal distance: Add score based on how far off the optimal connection distance */
		const float distance = Reflex::GetDistanceSq( graph.nodes[connection.from].position, graph.nodes[connection.to].position );
		const float difference = distance - float( GA_OptimalDistance * GA_OptimalDistance );
		graph.score += std::sqrt( abs( difference ) / 2.0f );
	}

	graph.score += intersections * 20;
}

void GeneticAlgorithm::CrossOver( const Graph& a, const Graph& b, const unsigned replaceIndex )
{
	//if( Reflex::RandomBool() )
	{
		Graph newChildA( a );
		Graph newChildB( b );

		CrossOver( newChildA, b );
		CrossOver( newChildB, a );

		m_population[replaceIndex] = newChildA;
		m_population[replaceIndex - 1] = newChildB;
	}
}

void GeneticAlgorithm::CrossOver( Graph& child, const Graph& parent2 )
{
	const unsigned nodesToSwap = Reflex::RandomInt( 5, parent2.nodes.size() / 4 );
	unsigned last = parent2.connections[Reflex::RandomInt( parent2.connections.size() - 1 )].from;

	for( unsigned i = 0U; i < nodesToSwap - 1; ++i )
	{
		child.nodes[last].position = parent2.nodes[last].position;

		if( parent2.nodes[last].connections.empty() )
			break;

		int test;

		do
		{
			const auto node = parent2.connections[Reflex::RandomInt( parent2.nodes[last].connections.size() - 1 )];
			test = Reflex::RandomBool() ? node.from : node.to;
		}
		while( test == last );

		last = test;
	}
}

void GeneticAlgorithm::Mutate( Graph& graph )
{
	const auto roll = Reflex::RandomInt( 100 );

	if( roll >= 60 )
	{
		// Randomly move a node by a small amount
		const auto offsetValue = 10.0f;
		auto& position = graph.nodes[Reflex::RandomInt( graph.nodes.size() - 1 )].position;
		position.x = Reflex::Clamp( position.x + ( Reflex::RandomFloat() - 0.5f ) * offsetValue, m_bounds.left, m_bounds.width );
		position.y = Reflex::Clamp( position.y + ( Reflex::RandomFloat() - 0.5f ) * offsetValue, m_bounds.top, m_bounds.height );
	}
	else if( roll >= 30 )
	{
		// Convert position to 80% to 120% of it's current value
		auto& position = graph.nodes[Reflex::RandomInt( graph.nodes.size() - 1 )].position;
		position.x = Reflex::Clamp( position.x * ( 0.8f + Reflex::RandomFloat() * 0.4f ), m_bounds.left, m_bounds.width );
		position.y = Reflex::Clamp( position.y * ( 0.8f + Reflex::RandomFloat() * 0.4f ), m_bounds.top, m_bounds.height );
	}
	else
	{
		// Randomly move a node
		const auto position = sf::Vector2f( m_bounds.left + Reflex::RandomFloat() * m_bounds.width, m_bounds.top + Reflex::RandomFloat() * m_bounds.height );
		graph.nodes[Reflex::RandomInt( graph.nodes.size() - 1 )].position = position;
	}
	
}