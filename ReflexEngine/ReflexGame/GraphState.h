#pragma once

#include "..\ReflexEngine\State.h"
#include "..\ReflexEngine\World.h"

#include "GraphNode.h"

class GraphState : public Reflex::Core::State
{
public:
	GraphState( Reflex::Core::StateManager& stateManager, Reflex::Core::Context context );

protected:
	void Render() final;
	bool Update( const sf::Time deltaTime ) final;
	bool ProcessEvent( const sf::Event& event ) final;

private:
	void ParseFile( const std::string& fileName );
	void GenerateGraphNodes();

private:
	Reflex::Core::World m_world;
	sf::FloatRect m_bounds;
};