// Includes
#include "..\ReflexEngine\Engine.h"

#include "GraphState.h"
#include "MenuState.h"
#include "SpacialHashMapDemo.h"

enum StateTypes : unsigned
{
	MenuStateType,
	GraphStateType,
	SpacialHashMapState,
	NumStateTypes,
};

// Entry point
int main( int argc, char** argv )
{
	srand( ( unsigned )time( 0 ) );

	Reflex::Core::Engine engine;

	engine.RegisterState< SpacialHashMapDemo >( SpacialHashMapState, true );
	engine.RegisterState< GraphState >( GraphStateType );
	engine.RegisterState< MenuState >( MenuStateType );

	engine.Run();

	return 0; 
}