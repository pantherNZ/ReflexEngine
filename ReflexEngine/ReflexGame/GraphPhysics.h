#pragma once

#include "..\ReflexEngine\System.h"

using namespace Reflex::Core;

// https://pdfs.semanticscholar.org/b8d3/bca50ccc573c5cb99f7d201e8acce6618f04.pdf
// https://en.wikipedia.org/wiki/Force-directed_graph_drawing#cite_note-kk89-11
// http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.13.8444&rep=rep1&type=pdf

class GraphPhysics : public Reflex::Systems::System
{
public:
	GraphPhysics( World& world, sf::FloatRect bounds ) : Reflex::Systems::System( world ), m_bounds( bounds ) { }

	void RegisterComponents() final;
	void Update( const float deltaTime ) final;
	void OnSystemStartup() final {}
	void OnSystemShutdown() final {}

	sf::FloatRect m_bounds;
};