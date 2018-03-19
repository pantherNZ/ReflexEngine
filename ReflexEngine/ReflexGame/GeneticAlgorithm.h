#pragma once

#include <SFML\System\Vector2.hpp>
#include <SFML\Graphics\Rect.hpp>

#include <string>
#include <set>
#include <vector>

// https://www.emis.de/journals/DM/v92/art5.pdf

class GeneticAlgorithm
{
public:
	GeneticAlgorithm( const std::string& fileName, const sf::FloatRect& bounds );

	struct Node
	{
		Node( sf::Vector2f pos, std::string str ) : position( pos ), label( str ) {}
		sf::Vector2f position;
		std::string label;
		std::vector< std::pair< unsigned, bool > > connections;
	};

	struct Connection
	{
		Connection( unsigned a, unsigned b ) : from( a ), to( b ) { }
		unsigned from;
		unsigned to;
	};

	struct Graph
	{
		std::vector< Node > nodes;
		std::vector< Connection > connections;
		float score = 0.0f;
	};

	typedef std::vector< Graph > Population;

	const Graph& GetBestGraph();
	const float GetAverageScore();
	void IteratePopulation();
	void AlgorithmicLayout();

protected:
	void ParseFile( const std::string& fileName );
	void ScoreGraph( Graph& graph );
	void CrossOver( const Graph& a, const Graph& b, const unsigned replaceIndex );
	void CrossOverA( Graph& child, const Graph& parent2 );
	void CrossOverB( Graph& child, const Graph& parent2 );
	void Mutate( Graph& graph );
	void Sort( Graph& graph );

protected:
	enum Variables
	{
		GA_Population = 10,
		GA_Iterations = 100,
		GA_CrossOverCount = 1,
		GA_MutationChance = 5,
		GA_OptimalDistance = 300,
	};

	Population m_population;
	sf::FloatRect m_bounds;
};