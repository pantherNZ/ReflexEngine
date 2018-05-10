#pragma once

#include "..\ReflexEngine\Engine.h"
#include "GameBoard.h"

using namespace Reflex::Core;

class PentagoGameState : public State
{
public:
	PentagoGameState( StateManager& stateManager, Context context );

protected:
	void Render() final;
	bool Update( const float deltaTime ) final;
	bool ProcessEvent( const sf::Event& event ) final;

private:
	sf::FloatRect m_bounds;
	World m_world;
	GameBoard m_board;
	sf::Text m_turnText[2];

	bool m_playerTurn = true;
	bool m_playerIsWhite = true;
};