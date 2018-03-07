// Includes
#include "..\ReflexEngine\Engine.h"

#include "GraphState.h"
#include "MenuState.h"

enum StateTypes : unsigned
{
	MenuStateType,
	GraphStateType,
	NumStateTypes,
};

// Entry point
int main( int argc, char** argv )
{
	Reflex::Core::Engine engine;

	engine.RegisterState< GraphState >( GraphStateType );
	engine.RegisterState< MenuState >( MenuStateType );
	engine.SetStartupState( GraphStateType );

	engine.Run();

	return 0;
}