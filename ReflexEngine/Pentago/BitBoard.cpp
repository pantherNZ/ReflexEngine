#include "BitBoard.h"

#include <intrin.h>  

void BoardData::Reset()
{
	data[0] = 0;
	data[1] = 0;
}

BoardType BoardData::GetTile( const Corner corner, const sf::Vector2u& index ) const
{
	return GetTile( data, corner, index );
}

BoardType BoardData::GetTile( const int64_t* data, const Corner corner, const sf::Vector2u& index ) const
{
	const auto mask = 1ULL << ( corner * 9 + index.y * 3 + index.x );
	return ( data[0] & mask ) ? BoardType::PlayerMarble : ( data[1] & mask ? BoardType::AIMarble : BoardType::Empty );
}

void BoardData::SetTile( const Corner corner, const sf::Vector2u& index, const BoardType player )
{
	const auto mask = 1ULL << ( corner * 9 + index.y * 3 + index.x );
	if( player == BoardType::Empty )
	{
		data[0] &= ~mask;
		data[1] &= ~mask;
	}
	else if( player == BoardType::PlayerMarble )
		data[0] |= mask;
	else
		data[1] |= mask;
}

void BoardData::RotateCorner( const Corner corner, const bool rotateLeft )
{
	int64_t copy[2] = { data[0], data[1] };

#define Rotate( to, from ) SetTile( corner, sf::Vector2u( to % 3, to / 3 ), GetTile( copy, corner, sf::Vector2u( from % 3, from / 3 ) ) );
	if( rotateLeft )
	{
		Rotate( 0, 2 );
		Rotate( 3, 1 );
		Rotate( 6, 0 );
		Rotate( 7, 3 );
		Rotate( 8, 6 );
		Rotate( 5, 7 );
		Rotate( 2, 8 );
		Rotate( 1, 5 );
	}
	else
	{
		Rotate( 0, 6 );
		Rotate( 1, 3 );
		Rotate( 2, 0 );
		Rotate( 5, 1 );
		Rotate( 8, 2 );
		Rotate( 7, 5 );
		Rotate( 6, 8 );
		Rotate( 3, 7 );
	}
#undef Rotate
}

BoardType BoardData::CheckWin()
{
	int a = 0;
	return CheckWin( false, a );
}

#define BitCount( x ) __popcnt64( x )

BoardType BoardData::CheckWin( const bool queryScore, int& score )
{
	BoardType result = Empty;
	uint64_t playerSide = data[0];
	uint64_t AISide = data[1];

	for( int i = 0; i < 32; i++ )
	{
		uint64_t winMap = winMaps[i];
		uint64_t player = ( playerSide & winMap );
		uint64_t AI = ( AISide & winMap );

		if( player && !AI )
		{
			if( queryScore )
				score += scoreMaps[CountSetBits( player )];

			if( player == winMap )
				result = PlayerMarble;
		}
		else if( !AI && player )
		{
			if( queryScore )
				score -= scoreMaps[CountSetBits( AI )];

			if( AI == winMap )
				result = AIMarble;
		}
	}

	return result;
}

void BoardData::PrintBoard()
{
	std::cout << "----------------\n";

	for( unsigned int y = 0U; y < 6; ++y )
	{
		std::cout << "| ";

		for( unsigned int x = 0U; x < 6; ++x )
		{
			const auto piece = GetTile( Corner( indexToCorner[y * 6 + x] ), sf::Vector2u( x % 3, y % 3 ) );
			std::cout << ( piece == BoardType::Empty ? ". " : ( piece == BoardType::PlayerMarble ? "1 " : "2 " ) );
		}

		std::cout << " |\n";
	}

	std::cout << "----------------\n\n";
}

/*
BoardType BoardData::CheckWin( int& score )
{
auto total = 0;
auto run = 0;

const auto ScoreRun = [&]()
{
score += abs( run ) <= 1 ? run : ( abs( run ) < 5 ? run * run * run : Reflex::Sign( run ) * std::numeric_limits< int >::max() );
run = 0;
};

const auto Get = [&]( const unsigned corner, const unsigned startIndex )
{
//const auto result = ( player == BoardType::Empty || player == data[corner][startIndex] ) ? data[corner][startIndex] : BoardType::Empty;
const auto value = data[corner][startIndex];

//if( player != BoardType::Empty && result != player && run )
if( value != Reflex::Sign( run ) && run )
{
ScoreRun();
run = 0;
}

run += value;

return value;
};

const auto Check = [&]( int value, bool first = false ) -> BoardType
{
const auto result = ( value == -5 ? PlayerMarble : ( value == 5 ? AIMarble : Empty ) );

if( !first || result != Empty )
{
total = 0;

if( run )
ScoreRun();
}
else
total = value;

return result;
};

const auto Vertical = [&]( Corner corner, const unsigned startIndex ) -> BoardType
{
const auto top = Get( corner, startIndex );
if( const auto result = Check( top + Get( corner, startIndex + 3 ) + Get( corner, startIndex + 6 ) + Get( corner + 2, startIndex ) + Get( corner + 2, startIndex + 3 ), true ) )
return result;
if( const auto result = Check( total - top + Get( corner + 2, startIndex + 6 ) ) )
return result;
return BoardType::Empty;
};

const auto Horizontal = [&]( Corner corner, const unsigned startIndex ) -> BoardType
{
const auto left = Get( corner, startIndex );
if( const auto result = Check( left + Get( corner, startIndex + 1 ) + Get( corner, startIndex + 2 ) + Get( corner + 1, startIndex ) + Get( corner + 1, startIndex + 1 ), true ) )
return result;
if( const auto result = Check( total - left + Get( corner + 1, startIndex + 2 ) ) )
return result;
return BoardType::Empty;
};

for( unsigned i = 0U; i < 3; ++i )
{
if( const auto result = Vertical( TopLeft, i ) )
return result;
if( const auto result = Vertical( TopRight, i ) )
return result;
if( const auto result = Horizontal( TopLeft, i * 3 ) )
return result;
if( const auto result = Horizontal( BottomLeft, i * 3 ) )
return result;
}

// Diagonals
if( const auto result = Check( Get( TopLeft, 0 ) + Get( TopLeft, 4 ) + Get( TopLeft, 8 ) + Get( BottomRight, 0 ) + Get( BottomRight, 4 ), true ) )
return result;
if( const auto result = Check( total - Get( TopLeft, 0 ) + Get( BottomRight, 8 ) ) )
return result;
if( const auto result = Check( Get( TopLeft, 1 ) + Get( TopLeft, 5 ) + Get( TopRight, 6 ) + Get( BottomRight, 1 ) + Get( BottomRight, 5 ) ) )
return result;
if( const auto result = Check( Get( TopLeft, 3 ) + Get( TopLeft, 7 ) + Get( BottomLeft, 2 ) + Get( BottomRight, 3 ) + Get( BottomRight, 7 ) ) )
return result;

if( const auto result = Check( Get( BottomLeft, 6 ) + Get( BottomLeft, 4 ) + Get( BottomLeft, 2 ) + Get( TopRight, 6 ) + Get( TopRight, 4 ), true ) )
return result;
if( const auto result = Check( total - Get( BottomLeft, 6 ) + Get( TopRight, 2 ) ) )
return result;
if( const auto result = Check( Get( BottomLeft, 3 ) + Get( BottomLeft, 1 ) + Get( TopLeft, 8 ) + Get( TopRight, 3 ) + Get( TopRight, 1 ) ) )
return result;
if( const auto result = Check( Get( BottomLeft, 7 ) + Get( BottomLeft, 5 ) + Get( BottomRight, 0 ) + Get( TopRight, 7 ) + Get( TopRight, 5 ) ) )
return result;

return BoardType::Empty;
}*/

/*
BoardType BoardData::CheckWin()
{
// Check along the main diagonal
for( unsigned i = 0U; i < 6; ++i )
{
const auto result = CheckWinAtIndex( i, i );
if( result != BoardType::Empty )
return result;
}

// Also check the middle row that is missed
return CheckWinAtIndex( 5, 0, true );
}

BoardType BoardData::CheckWinAtIndex( const int locX, const int locY, const bool diagonalsOnly, const bool straightsOnly, std::vector< unsigned >& runsPlayer, std::vector< unsigned >& runsAI )
{
std::array< unsigned, 4 > countersPlayer = { 0, 0, 0, 0 };
std::array< unsigned, 4 > countersAI = { 0, 0, 0, 0 };

const auto AppendRuns = [&]( unsigned index )
{
if( countersPlayer[index] > 0U )
{
runsPlayer.push_back( countersPlayer[index] );
countersPlayer[index] = 0U;
}

if( countersAI[index] > 0U )
{
runsAI.push_back( countersAI[index] );
countersAI[index] = 0U;
}
};

const auto Check = [&]( unsigned index, unsigned x, unsigned y )
{
if( data[y][x] == BoardType::Empty )
{
AppendRuns( index );
return BoardType::Empty;
}

auto* counter = data[y][x] == BoardType::PlayerMarble ? &countersPlayer : &countersAI;
( *counter )[index]++;

if( counter->at( index ) >= 5 )
{
AppendRuns( index );
return data[y][x];
}

return BoardType::Empty;
};

for( int offset = 0U; offset < 6; ++offset )
{
// X axis
if( !diagonalsOnly )
{
if( auto result = Check( 0, offset, locY ) )
return result;

// Y axis
if( auto result = Check( 1, locX, offset ) )
return result;
}

// Top left to bot right diagonal
if( !straightsOnly )
{
if( abs( locX - locY ) <= 1 )
if( auto result = Check( 2, std::min( 5, std::max( 0, locX - locY ) + offset ), std::min( 5, std::max( 0, locY - locX ) + offset ) ) )
return result;

// Bot left to top right diagonal
if( abs( ( 5 - locX ) - locY ) <= 1 )
if( auto result = Check( 3, std::min( 5, std::max( 0, abs( locX + locY - std::min( 5, locX + locY ) ) ) + offset ), std::max( 0, std::min( 5, locX + locY ) - offset ) ) )
return result;
}
}

for( unsigned i = 0U; i < 4; ++i )
AppendRuns( i );

return BoardType::Empty;
}

unsigned BoardData::ScoreBoard( const BoardType player )
{
auto finalScore = 0U;

std::vector< unsigned > countersAI, countersPlayer;

const auto Score = [&]( BoardType result )
{
if( result == player )
{
finalScore += std::numeric_limits< unsigned >::max();
return;
}

auto* counters = player == BoardType::AIMarble ? &countersAI : &countersPlayer;

for( auto& count : *counters )
finalScore += ( count * count * count * 10U );

countersAI.clear();
countersPlayer.clear();
};

// Check along the main diagonal (straights only)
for( unsigned i = 0U; i < 6; ++i )
Score( CheckWinAtIndex( i, i, false, true, countersPlayer, countersAI ) );

// Check along the diagonals
Score( CheckWinAtIndex( 0, 0, true, false, countersPlayer, countersAI ) );
Score( CheckWinAtIndex( 0, 1, true, false, countersPlayer, countersAI ) );
Score( CheckWinAtIndex( 1, 0, true, false, countersPlayer, countersAI ) );

Score( CheckWinAtIndex( 0, 5, true, false, countersPlayer, countersAI ) );
Score( CheckWinAtIndex( 0, 4, true, false, countersPlayer, countersAI ) );
Score( CheckWinAtIndex( 1, 5, true, false, countersPlayer, countersAI ) );

return finalScore;
}*/