#include "Game.h"
#include "GraphState.h"

Game::Game() : Engine()
{
	RegisterStates();

	m_stateManager.PushState( GetStartupState() );
}

unsigned Game::GetStartupState() const
{
	return States::GraphState;
}

void Game::RegisterStates()
{
	//m_stateManager.RegisterState< MenuState >( States::MenuState );
	m_stateManager.RegisterState< GraphState >( States::GraphState );
}

void Game::OnPostSetup()
{

}