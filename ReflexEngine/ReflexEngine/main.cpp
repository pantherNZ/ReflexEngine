// Includes
#include "Engine.h"

// Entry point
int main( int argc, char** argv )
{
	// Engine instance
	Reflex::Core::Engine engine;

	if( !engine.Initialise() )
	{
		Log( Reflex::ELogType::CRIT, "Reflex Engine failed to initialise!" );
		return -1;
	}

	engine.Run();

	return 0;
}