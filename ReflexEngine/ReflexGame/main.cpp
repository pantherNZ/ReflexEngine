// Includes
#include "Game.h"

// Entry point
int main( int argc, char** argv )
{
	try
	{
		Game game;
		game.Run();
	}
	catch( std::exception& e )
	{
		Reflex::LOG_CRIT( "EXCEPTION: " + std::to_string( *e.what() ) + "\n" );
	}

	return 0;
}