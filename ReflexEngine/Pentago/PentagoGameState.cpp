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
	, m_board( m_world, m_playerIsWhite )
{
	context.fontManager->LoadResource( Reflex::ResourceID::ArialFontID, "Data/Fonts/arial.ttf" );

	m_turnText[0] = sf::Text( "Your turn", context.fontManager->GetResource( Reflex::ResourceID::ArialFontID ), 40U );
	m_turnText[1] = sf::Text( "Computer's turn", context.fontManager->GetResource( Reflex::ResourceID::ArialFontID ), 40U );

	for( unsigned i = 0U; i < 2; ++i )
	{
		m_turnText[i].setPosition( m_board.m_bounds.centre - sf::Vector2f( 0.0f, m_board.m_bounds.halfSize.y + 50.0f ) );
		m_turnText[i].setFillColor( sf::Color::White );
		Reflex::CenterOrigin( m_turnText[i] );
	}
}

void PentagoGameState::Render()
{
	m_world.Render();

	GetContext().window->draw( m_turnText[m_playerTurn ? 0 : 1] );
}

bool PentagoGameState::Update( const float deltaTime )
{
	m_world.Update( deltaTime );
	return true;
}

bool PentagoGameState::ProcessEvent( const sf::Event& event )
{
	m_world.ProcessEvent( event );

	if (event.type == sf::Event::MouseButtonPressed )
	{
		m_board.PlaceMarble( false, Reflex::ToVector2f( sf::Mouse::getPosition( *GetContext().window ) ) );
	}

	return true;
}