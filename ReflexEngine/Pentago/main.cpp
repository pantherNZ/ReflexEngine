// Includes
#include "..\ReflexEngine\Engine.h"

#include "PentagoGameState.h"

enum StateTypes : unsigned
{
	PentagoMenuStateType,
	PentagoGameStateType,
	NumStateTypes,
};

// Entry point
int main( int argc, char** argv )
{
	srand( ( unsigned )time( 0 ) );

	Reflex::Core::Engine engine;

	//engine.RegisterState< PentagoMenuState >( PentagoMenuStateType, true );
	engine.RegisterState< PentagoGameState >( PentagoGameStateType, true );

	engine.Run();

	return 0;
}