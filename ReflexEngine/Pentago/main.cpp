// Includes
#include "..\ReflexEngine\Engine.h"

#include "PentagoGameState.h"
#include "PentagoMenuState.h"
#include "Resources.h"

// Entry point
int main( int argc, char** argv )
{
	srand( ( unsigned )time( 0 ) );

	Reflex::Core::Engine engine;

	engine.RegisterState< PentagoMenuState >( PentagoMenuStateType, false );
	engine.RegisterState< PentagoGameState >( PentagoGameStateType, true );
	engine.RegisterState< SetDifficultyState >( SetDifficultyStateType );
	engine.RegisterState< InGameMenuState >( InGameMenuStateType );

	engine.Run();

	return 0;
}