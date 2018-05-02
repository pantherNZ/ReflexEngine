#include "PentagoGameState.h"

namespace Reflex
{
	enum class ResourceID : unsigned short
	{
		ArialFontID,
		GraphNodeTextureID,
	};
}

PentagoGameState::PentagoGameState( StateManager& stateManager, Context context )
	: State( stateManager, context )
	, m_bounds( 0.0f, 0.0f, ( float )context.window->getSize().x, ( float )context.window->getSize().y )
	, m_world( *context.window, m_bounds, 100 )
	, m_grid( m_world )
{

}

void PentagoGameState::Render()
{
	m_world.Render();
}

bool PentagoGameState::Update( const float deltaTime )
{
	m_world.Update( deltaTime );
	return true;
}

bool PentagoGameState::ProcessEvent( const sf::Event& event )
{
	if (event.type == sf::Event::MouseButtonPressed )
	{
		m_grid.AddGridObject( false, Reflex::ToVector2f( sf::Mouse::getPosition( *GetContext().window ) ) );
	}

	return true;
}