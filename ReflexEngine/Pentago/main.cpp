// Includes
#include "..\ReflexEngine\Engine.h"

#include "vld.h"

#include "PentagoGameState.h"
#include "PentagoMenuState.h"
#include "Resources.h"

// Entry point
int main( int argc, char** argv )
{
	srand( ( unsigned )time( 0 ) );

	Reflex::Core::Engine engine;

	engine.RegisterState< PentagoMenuState >( PentagoMenuStateType, true );
	engine.RegisterState< PentagoGameState >( PentagoGameStateType, false );
	engine.RegisterState< SetDifficultyState >( SetDifficultyStateType, false );

	engine.Run();

	return 0;
}