#pragma once

#include "..\ReflexEngine\Engine.h"
#include "..\ReflexEngine\QuadTree.h"

using namespace Reflex::Core;

enum class GameState
{
	PlayerTurn,
	PlayerSpinSelection,
	PlayerCornerSpinning,
	AITurn,
	AICornerSpinning,
	PlayerWin,
	AIWin,
};

class GameBoard
{
public:
	GameBoard( World& world, class PentagoGameState& gameState, const bool playerIsWhite );

	void PlaceAIMarble( const sf::Vector2u index );
	void ToggleArrows( const bool show );
	void RotateCorner( const unsigned x, const unsigned y, const bool rotateLeft );
	bool CheckWin( const int locX, const int locY, const bool isPlayer );
	void ForEachSlot( std::function< void( const ObjectHandle& obj, const sf::Vector2u index ) > callback );

public:
	World& m_world;
	sf::FloatRect m_boardBounds;
	float m_marbleSize = 0.0f;

	GameState m_boardState = GameState::PlayerTurn;
	PentagoGameState& m_gameState;

	ObjectHandle m_selectedMarble;
	ObjectHandle m_playerMarble;
	ObjectHandle m_gameBoard;
	ObjectHandle m_cornerArrows[8];
};