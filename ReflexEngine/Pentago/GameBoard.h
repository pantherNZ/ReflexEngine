#pragma once

#include "..\ReflexEngine\Engine.h"
#include "..\ReflexEngine\QuadTree.h"
#include "BitBoard.h"

using namespace Reflex::Core;

enum class GameState
{
	PlayerTurn,
	PlayerSpinSelection,
	PlayerCornerSpinning,
	AITurn,
	AISpinSelection,
	AICornerSpinning,
	PlayerWin,
	AIWin,
	NumStates,
};

class GameBoard
{
public:
	GameBoard( World& world, class PentagoGameState& gameState, const bool playerIsWhite );

	void PlacePlayerMarble( Reflex::Core::ObjectHandle object, Reflex::Core::GridHandle cornerGrid, const sf::Vector2u& cornerIndex, const sf::Vector2u& index );
	void RemoveAIMarble( Reflex::Core::ObjectHandle object, Reflex::Core::GridHandle cornerGrid, const sf::Vector2u& cornerIndex, const sf::Vector2u& index );

	void PlaceAIMarble( const Corner corner, const sf::Vector2u& index );

	void ToggleArrows( const bool show );
	void RotateCorner( const Corner corner, const bool rotateLeft );
	GameState CheckWin();
	void ForEachSlot( std::function< void( const ObjectHandle& obj, const sf::Vector2u index ) > callback );

public:
	World& m_world;
	sf::FloatRect m_boardBounds;
	float m_marbleSize = 0.0f;

	GameState m_boardState = GameState::PlayerTurn;
	PentagoGameState& m_gameState;

	ObjectHandle m_selectedMarble;
	ObjectHandle m_playerMarbles[2];
	bool m_classicMode = false;
	bool m_doubleRotate = false;
	ObjectHandle m_gameBoard;
	ObjectHandle m_cornerArrows[8];
	ObjectHandle m_skipButton;

	BoardData m_boardData;
};