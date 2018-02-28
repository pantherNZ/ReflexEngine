// Includes
#include "..\ReflexEngine\Engine.h"

#include "GraphState.h"
#include "MenuState.h"

// Entry point
int main( int argc, char** argv )
{
	try
	{
		Reflex::Core::Engine engine;

		engine.RegisterState< GraphState >( 0 );
		engine.RegisterState< MenuState >( 0 );

		engine.Run();
	}
	catch( std::exception& e )
	{
		Reflex::LOG_CRIT( "EXCEPTION: " + std::to_string( *e.what() ) + "\n" );
	}

	return 0;
}