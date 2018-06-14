#include "GeneticAlgorithm.h"

#include <map>
#include <fstream>

#include "..\ReflexEngine\Utility.h"
#include <unordered_set>

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
					m_population.back().nodes.back().connections.push_back( std::make_pair( lastAdded, true ) );
					m_population.back().nodes[lastAdded].connections.push_back( std::make_pair( index, false ) );
				}
				else
				{
					// Add to connection list
					m_population.back().connections.push_back( Connection( found->second, lastAdded ) );
					m_population.back().nodes[found->second].connections.push_back( std::make_pair( lastAdded, true ) );
					m_population.back().nodes[lastAdded].connections.push_back( std::make_pair( found->second, false ) );
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
	Graph newChildA( a );
	Graph newChildB( b );

	if( Reflex::RandomBool() )
	{
		CrossOverA( newChildA, b );
		CrossOverA( newChildB, a );
	}
	else
	{
		CrossOverB( newChildA, b );
		CrossOverB( newChildB, a );
	}

	m_population[replaceIndex] = newChildA;
	m_population[replaceIndex - 1] = newChildB;
}

void GeneticAlgorithm::CrossOverA( Graph& child, const Graph& parent2 )
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

void GeneticAlgorithm::CrossOverB( Graph& child, const Graph& parent2 )
{
	unsigned nodeIndex = 0U;
	unsigned connections = std::numeric_limits< unsigned >::max();

	for( unsigned i = 0U; i < 5; ++i )
	{
		const auto index = Reflex::RandomInt( parent2.nodes.size() - 1 );
		const auto node = parent2.nodes[index];

		if( node.connections.size() >= connections )
		{
			connections = node.connections.size();
			nodeIndex = index;
		}
	}

	for( const auto& connection : parent2.nodes[nodeIndex].connections )
		child.nodes[connection.first].position = parent2.nodes[connection.first].position;
}

void GeneticAlgorithm::Mutate( Graph& graph )
{
	const auto roll = Reflex::RandomInt( 100 );

	if( roll >= 75 )
	{
		// Randomly move a node by a small amount
		const auto offsetValue = 10.0f;
		auto& position = graph.nodes[Reflex::RandomInt( graph.nodes.size() - 1 )].position;
		position.x = Reflex::Clamp( position.x + ( Reflex::RandomFloat() - 0.5f ) * offsetValue, m_bounds.left, m_bounds.width );
		position.y = Reflex::Clamp( position.y + ( Reflex::RandomFloat() - 0.5f ) * offsetValue, m_bounds.top, m_bounds.height );
	}
	else if( roll >= 50 )
	{
		// Convert position to 80% to 120% of it's current value
		auto& position = graph.nodes[Reflex::RandomInt( graph.nodes.size() - 1 )].position;
		position.x = Reflex::Clamp( position.x * ( 0.8f + Reflex::RandomFloat() * 0.4f ), m_bounds.left, m_bounds.width );
		position.y = Reflex::Clamp( position.y * ( 0.8f + Reflex::RandomFloat() * 0.4f ), m_bounds.top, m_bounds.height );
	}
	else if( roll >= 25)
	{
		// Randomly move a node
		const auto position = sf::Vector2f( m_bounds.left + Reflex::RandomFloat() * m_bounds.width, m_bounds.top + Reflex::RandomFloat() * m_bounds.height );
		graph.nodes[Reflex::RandomInt( graph.nodes.size() - 1 )].position = position;
	}
	else
	{
		// Resize a connection
		const auto connection = Reflex::RandomInt( graph.connections.size() - 1 );
		const auto direction = graph.nodes[graph.connections[connection].from].position - graph.nodes[graph.connections[connection].to].position;
		const auto length = Reflex::GetMagnitude( direction );
		const auto directionNorm = direction / length;
		const auto correctionDist = GA_OptimalDistance - length;
		graph.nodes[graph.connections[connection].from].position += directionNorm * correctionDist / 2.0f;
		graph.nodes[graph.connections[connection].to].position -= directionNorm * correctionDist / 2.0f;
	}
}

void GeneticAlgorithm::AlgorithmicLayout()
{
	// Sort by number of connections
	auto& graph = m_population.front();
	graph.score = 0.0f;

	auto copy( graph );

	Sort( graph );

	const auto graphNodes = graph.nodes.size();
	const float boundary = 80.0f;

	for( unsigned i = 0U; i < graphNodes; ++i )
	{
		if( i < graphNodes / 4 )
		{
			const auto interp = i / ( graphNodes / 4.0f );
			graph.nodes[i].position = sf::Vector2f( boundary + ( m_bounds.width - boundary * 2.0f ) * interp, boundary );
		}
		else if( i < graphNodes / 2 )
		{
			const auto interp = ( i - ( graphNodes / 4 ) ) / ( graphNodes / 4.0f );
			graph.nodes[i].position = sf::Vector2f( m_bounds.width - boundary, boundary + ( m_bounds.height - boundary * 2.0f ) * interp );

		}
		else if( i < ( graphNodes * 3 ) / 4 )
		{
			const auto interp = ( i - ( graphNodes / 2 ) ) / ( graphNodes / 4.0f );
			graph.nodes[i].position = sf::Vector2f( boundary + ( m_bounds.width - boundary * 2.0f ) * interp, m_bounds.height - boundary );
		}
		else
		{
			const auto interp = ( i - ( graphNodes * 3 ) / 4 ) / ( graphNodes / 4.0f );
			graph.nodes[i].position = sf::Vector2f( boundary, boundary + ( m_bounds.height - boundary * 2.0f ) * interp );
		}
	}

	const auto centre = sf::Vector2f( m_bounds.width / 2.0f, m_bounds.height / 2.0f );
	graph.nodes[0].position = centre;

	const auto size = graph.nodes[0].connections.size();
	const auto angle = PI2 / size;

	for( unsigned i = 0U; i < size; ++i )
	{
		auto& connection = graph.nodes[0].connections[i];
		graph.nodes[connection.first].position = centre + Reflex::VectorFromAngle( TODEGREES( angle * i ), GA_OptimalDistance - 20.0f + graph.nodes[connection.first].connections.size() * 20.0f );
	}
}

void GeneticAlgorithm::Sort( Graph& graph )
{
	std::sort( m_population.back().nodes.begin(), m_population.back().nodes.end(), []( const Node& left, const Node& right )
	{
		return left.connections.size() > right.connections.size();
	} );

	graph.connections.clear();

	for( unsigned i = 0U; i < m_population.back().nodes.size(); ++i )
	{
		for( unsigned j = 0U; j < m_population.back().nodes[i].connections.size(); ++j )
		{
			Connection con( i, m_population.back().nodes[i].connections[j].first );

			if( std::find_if( graph.connections.begin(), graph.connections.end(), [&con]( const Connection& connection )
			{
				return con.from == connection.from && con.to == connection.to;
			} ) == graph.connections.end() )
			{
				graph.connections.push_back( con );
			}
		}
	}
}