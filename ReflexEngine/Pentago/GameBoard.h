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
	AISpinSelection,
	AICornerSpinning,
	PlayerWin,
	AIWin,
	NumStates,
};

enum BoardType : char
{
	PlayerMarble = -1, 
	Empty = 0,
	AIMarble = 1,
};

enum Corner : char
{
	None = -1,
	TopLeft,
	TopRight,
	BottomLeft,
	BottomRight,
	NumCorners,
};

class BoardData
{
public:
	BoardData() { Reset(); }
	void Reset();
	BoardType GetTile( const Corner corner, const sf::Vector2u& index ) const;
	void SetTile( const Corner corner, const sf::Vector2u& index, const BoardType player );
	void RotateCorner( const Corner corner, const bool rotateLeft );
	void PrintBoard();
	BoardType CheckWin();
	BoardType CheckWin( int& score );

	//BoardType CheckWinAtIndex( const int locX, const int locY, const bool diagonalsOnly = false, const bool straightsOnly = false,
	//	std::vector< unsigned >& runsPlayer = std::vector< unsigned >(),
	//	std::vector< unsigned >& runsAI = std::vector< unsigned >() );

private:
	std::array< std::array< BoardType, 9 >, 4 > data;
};

class GameBoard
{
public:
	GameBoard( World& world, class PentagoGameState& gameState, const bool playerIsWhite );

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
	ObjectHandle m_playerMarble;
	ObjectHandle m_gameBoard;
	ObjectHandle m_cornerArrows[8];
	ObjectHandle m_skipButton;

	BoardData m_boardData;
};