#pragma once

#include "..\ReflexEngine\State.h"
#include "..\ReflexEngine\World.h"
#include <SFML\System\Vector2.hpp>

#include "GraphNode.h"

using namespace Reflex::Core;

class GraphState : public State
{
public:
	GraphState( StateManager& stateManager, Context context );

protected:
	void Render() final;
	bool Update( const sf::Time deltaTime ) final;
	bool ProcessEvent( const sf::Event& event ) final;

private:
	void ParseFile( const std::string& fileName );
	void GenerateGraphNodes();
	ObjectHandle CreateGraphObject( const sf::Vector2f& position, const std::string& label );

private:
	World m_world;
	sf::FloatRect m_bounds;
};