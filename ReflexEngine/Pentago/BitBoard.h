#pragma once

#include "..\ReflexEngine\Engine.h"

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
	BoardType CheckWin( const bool queryScore, int& score );

	int64_t CountSetBits( int64_t n )
	{
		int64_t count = 0ULL;
		while( n )
		{
			n &= ( n - 1 );
			count++;
		}
		return count;
	}

	BoardType GetTile( const int64_t* data, const Corner corner, const sf::Vector2u& index ) const;

public:
	int64_t data[2]; // One board for each player

	const int indexToBit[36] =
	{
		0,  1,  2,  9, 10, 11,
		3,  4,  5, 12, 13, 14,
		6,  7,  8, 15, 16, 17,
		18, 19, 20, 27, 28, 29,
		21, 22, 23, 30, 31, 32,
		24, 25, 26, 33, 34, 35,
	};

	const int indexToCorner[36] =
	{
		0, 0, 0, 1, 1, 1,
		0, 0, 0, 1, 1, 1,
		0, 0, 0, 1, 1, 1,
		2, 2, 2, 3, 3, 3,
		2, 2, 2, 3, 3, 3,
		2, 2, 2, 3, 3, 3,
	};

#define S(x) ( 1ULL << indexToBit[x] )
	const uint64_t bitMasks[36] =
	{
		S( 0 ),  S( 1 ),  S( 2 ),  S( 3 ),  S( 4 ),  S( 5 ),
		S( 6 ),  S( 7 ),  S( 8 ),  S( 9 ),  S( 10 ), S( 11 ),
		S( 12 ), S( 13 ), S( 14 ), S( 15 ), S( 16 ), S( 17 ),
		S( 18 ), S( 19 ), S( 20 ), S( 21 ), S( 22 ), S( 23 ),
		S( 24 ), S( 25 ), S( 26 ), S( 27 ), S( 28 ), S( 29 ),
		S( 30 ), S( 31 ), S( 32 ), S( 33 ), S( 34 ), S( 35 ),
	};
#undef S

#define S(a,b,c,d,e) ((1ULL<<(a)) | (1ULL<<(b)) | (1ULL<<(c)) | (1ULL<<(d)) | (1ULL<<(e)))
	const uint64_t winMaps[32] =
	{
		//horizontal
		S( 0,  1,  2, 9, 10 ), S( 1,  2, 9, 10, 11 ),
		S( 3,  4,  5, 12, 13 ), S( 4,  5, 12, 13, 14 ),
		S( 6,  7,  8, 15, 16 ), S( 7,  8, 15, 16, 17 ),
		S( 18, 19, 20, 27, 28 ), S( 19, 20, 27, 28, 29 ),
		S( 21, 22, 23, 30, 31 ), S( 22, 23, 30, 31, 32 ),
		S( 24, 25, 26, 33, 34 ), S( 25, 26, 33, 34, 35 ),
		//vertical
		S( 0,  3,  6, 18, 21 ), S( 3,  6, 18, 21, 24 ),
		S( 1,  4,  7, 19, 22 ), S( 4,  7, 19, 22 , 25 ),
		S( 2,  5,  8, 20, 23 ), S( 5,  8, 20, 23, 26 ),
		S( 9, 12, 15, 27, 30 ), S( 12, 15, 27, 30, 33 ),
		S( 10, 13, 16, 28, 31 ), S( 13, 16, 28, 31, 34 ),
		S( 11, 14, 17, 29, 32 ), S( 14, 17, 29, 32, 35 ),
		//diagonal
		S( 0,  4,  8, 27, 31 ), S( 4,  8, 27, 31, 35 ),
		S( 1,  5, 15, 28, 32 ), S( 3,  7, 20, 30, 34 ),
		S( 24, 22, 20, 15, 13 ), S( 22, 20, 15, 13, 11 ),
		S( 21, 19, 8, 12, 10 ), S( 25, 23, 27, 16, 14 ),
	};
#undef S

	const int16_t scoreMaps[6] = { 0, 1, 3, 9, 27, 127 };
};