#pragma once

#include "..\ReflexEngine\State.h"
#include "..\ReflexEngine\World.h"
#include <SFML\System\Vector2.hpp>

#include "GraphNode.h"
#include "GeneticAlgorithm.h"

using namespace Reflex::Core;

class GraphState : public State
{
public:
	GraphState( StateManager& stateManager, Context context );

protected:
	void Render() final;
	bool Update( const float deltaTime ) final;
	bool ProcessEvent( const sf::Event& event ) final;

private:	
	ObjectHandle CreateGraphObject( const sf::Vector2f& position, const std::string& label );
	void RebuildRenderGraph();

private:
	sf::FloatRect m_bounds;
	World m_world;

	GeneticAlgorithm m_ga;
	sf::Text m_gaInfo;

	enum
	{
		GA_UpdateIntervalMS = 0,
		GA_RenderIntervalMS = 100,
	};

	float m_gaUpdateTimer = 0.0f;
	float m_gaRenderTimer = 0.0f;
};