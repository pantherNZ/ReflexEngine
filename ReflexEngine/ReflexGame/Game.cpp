#include "Game.h"
#include "GraphState.h"

Game::Game() : Engine()
{
	RegisterStates();

	m_stateManager.PushState( GetStartupState() );
}

unsigned Game::GetStartupState() const
{
	return Scenes::GraphScene;
}

void Game::RegisterStates()
{
	//m_stateManager.RegisterState< MenuState >( Scenes::MenuScene );
	m_stateManager.RegisterState< GraphState >( Scenes::GraphScene );
}

void Game::OnPostSetup()
{

}