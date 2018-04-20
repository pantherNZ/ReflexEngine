#include "MenuState.h"

MenuState::MenuState( Reflex::Core::StateManager& stateManager, Reflex::Core::Context context )
	: State( stateManager, context )
{

}

void MenuState::Render()
{

}

bool MenuState::Update( const float deltaTime )
{
	return false;
}

bool MenuState::ProcessEvent( const sf::Event& event )
{
	return false;
}