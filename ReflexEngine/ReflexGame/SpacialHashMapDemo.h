#pragma once

#include "..\ReflexEngine\State.h"
#include "..\ReflexEngine\World.h"

using namespace Reflex::Core;

class SpacialHashMapDemo : public State
{
public:
	SpacialHashMapDemo( StateManager& stateManager, Context context );

protected:
	void Render() final;
	bool Update( const float deltaTime ) final;
	bool ProcessEvent( const sf::Event& event ) final;

private:
	sf::FloatRect m_bounds;
	World m_world;
	const unsigned m_objectCount;
};