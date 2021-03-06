#pragma once

#include "..\ReflexEngine\Engine.h"
#include "GameBoard.h"

using namespace Reflex::Core;

enum MoveType
{
	Default,
	Destroyer,
	Spinner
};

struct Move
{
	Corner corner;
	sf::Vector2u index;
	Corner rotation;
	bool leftRotation;
	MoveType type;
};

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

public:
	static unsigned m_AIDifficulty;

	Move m_nextAIMove;
	std::unique_ptr< sf::Thread > m_AIThread;
	mutable bool m_AIThreadFinished = false;
	BoardData m_AIBoard;

private:
	sf::FloatRect m_bounds;
	World m_world;

	bool m_playerIsWhite = true;
	bool m_playerTurn = true;
	bool m_gameOver;

	float m_AITimer = 0.0f;

	GameBoard m_board;
	sf::Text m_text[4];
};