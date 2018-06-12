#pragma once

#include "..\ReflexEngine\Engine.h"
#include "GameBoard.h"

using namespace Reflex::Core;

class PentagoGameState : public State
{
public:
	PentagoGameState( StateManager& stateManager, Context context );

	void SetTurn( const bool playerTurn );
	void GameOver( const bool playerWin );

protected:
	void Render() final;
	bool Update( const float deltaTime ) final;
	bool ProcessEvent( const sf::Event& event ) final;

	void AITurn();

private:
	sf::FloatRect m_bounds;
	World m_world;

	bool m_playerTurn = true;
	bool m_playerIsWhite = true;
	bool m_gameOver;

	float m_AITimer = 0.0f;

	GameBoard m_board;
	sf::Text m_text[4];
};