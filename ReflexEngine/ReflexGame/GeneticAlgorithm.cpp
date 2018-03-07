#include "GeneticAlgorithm.h"
#include "GraphNode.h"
#include "..\ReflexEngine\World.h"

void GeneticAlgorithm::RegisterComponents()
{
	RequiresComponent< GraphNode >();
}

void GeneticAlgorithm::Update( const sf::Time deltaTime )
{

}
/*
#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <array>
#include <algorithm>

struct Node
{
	Node( float _x, float _y, std::string str ) : x( _x ), y( _y ), text( str ) { }
	float x = 0.0f;
	float y = 0.0f;
	std::string text;
	std::set< Node* > connections;
};

namespace
{
	const int nodeCount = 20;
	const int connectionsCount = 40;// rand() % 40;
	const int screenWidth = 1920;
	const int screenHeight = 1080;

	const int GA_population = 20;
	const int GA_iterations = 100;
	const int GA_crossOverCount = 2;
}

typedef std::pair< std::vector< Node >, float > Graph;
typedef std::vector< Graph > Population;

void InitialiseGraph( Graph& graph, const bool initialiseConnections )
{
	graph.first.reserve( nodeCount );

	for( int i = 0; i < nodeCount; ++i )
		graph.first.emplace_back( Node( float( rand() % screenWidth ), float( rand() % screenHeight ), "test" + std::to_string( i ) ) );

	if( initialiseConnections )
	{
		for( int i = 0; i < connectionsCount; ++i )
		{
			const int connectionFrom = rand() % ( nodeCount - 1 );
			graph.first[connectionFrom].connections.insert( &graph.first[connectionFrom + ( rand() % ( nodeCount - connectionFrom ) )] );
		}
	}
}

void ScoreGraph( Graph& graph )
{
	graph.second = 0.0f;

	for( auto& node : graph.first )
	{
		for( auto& othernode : graph.first )
		{
			if( &node != &othernode )
			{
				const float distX = ( node.x - othernode.x );
				const float distY = ( node.y - othernode.y );

				graph.second += sqrt( distX * distX + distY * distY );
			}
		}
	}
}

int main()
{
	srand( 0 );

	Population population( 1 );
	population.reserve( GA_population );

	for( int i = 0; i < GA_population; ++i )
	{
		if( i > 0 ) 
			population.emplace_back( Graph( population[0] ) );

		InitialiseGraph( population.back(), i == 0 );
	}

	while( true )
	{
		// Score the graph using heuristics
		for( Graph& graph : population )
			ScoreGraph( graph );

		// Sort by score
		std::sort( population.begin(), population.end(), [&]( const Graph& graphA, const Graph& graphB )
		{
			return graphA.second > graphB.second;
		} );

		// Replace the weakest with new children from the strongest
		for( int i = 0; i < GA_crossOverCount; ++i )
		{
			population[i].
		}
	}

	int temp;
	std::cin >> temp;
}
*/