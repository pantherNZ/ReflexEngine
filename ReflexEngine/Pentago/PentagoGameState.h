#pragma once

#include "..\ReflexEngine\Engine.h"
#include "GameBoard.h"

using namespace Reflex::Core;

struct Move
{
	Corner corner;
	sf::Vector2u index;
	Corner rotation;
	bool leftRotation;
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
	//int NegaMax( BoardData& board, const unsigned depth, int alpha, int beta, const BoardType player, std::vector< std::pair< sf::Vector2u, unsigned > >& bestMove );

public:
	static unsigned m_AISearchDepth;

private:
	sf::FloatRect m_bounds;
	World m_world;

	bool m_playerIsWhite = true;
	bool m_playerTurn = true;
	bool m_gameOver;

	float m_AITimer = 0.0f;

	GameBoard m_board;
	sf::Text m_text[4];

	Move m_nextAIMove;
	std::unique_ptr< sf::Thread > m_AIThread;
	BoardData m_AIBoard;
};