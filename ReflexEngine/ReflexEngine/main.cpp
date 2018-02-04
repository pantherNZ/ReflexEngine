// Includes
#include "Engine.h"

// Entry point
int main( int argc, char** argv )
{
	try
	{
		Reflex::Core::Engine engine;
		engine.Run();
	}
	catch( std::exception& e )
	{
		Reflex::LOG_CRIT( "EXCEPTION: " + std::to_string( *e.what() ) + "\n" );
	}

	return 0;
}