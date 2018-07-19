#pragma once

#include "..\ReflexEngine\Engine.h"

using namespace Reflex::Core;

class PentagoMenuState : public State
{
public:
	PentagoMenuState( StateManager& stateManager, Context context );

protected:
	void Render() final;
	bool Update( const float deltaTime ) final;
	bool ProcessEvent( const sf::Event& event ) final;

private:
	sf::FloatRect m_bounds;
	World m_world;

	ObjectHandle m_helpDisplay;
};

class SetDifficultyState : public State
{
public:
	SetDifficultyState( StateManager& stateManager, Context context );

protected:
	void Render() final;
	bool Update( const float deltaTime ) final;
	bool ProcessEvent( const sf::Event& event ) final;

private:
	sf::FloatRect m_bounds;
	World m_world;
};

class InGameMenuState : public State
{
public:
	InGameMenuState( StateManager& stateManager, Context context );

protected:
	void Render() final;
	bool Update( const float deltaTime ) final;
	bool ProcessEvent( const sf::Event& event ) final;

private:
	sf::FloatRect m_bounds;
	//World m_world;
};