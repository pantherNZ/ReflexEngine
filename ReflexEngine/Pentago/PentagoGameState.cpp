#include "PentagoGameState.h"
#include "Resources.h"

PentagoGameState::PentagoGameState( StateManager& stateManager, Context context )
	: State( stateManager, context )
	, m_bounds( 0.0f, 0.0f, ( float )context.window->getSize().x, ( float )context.window->getSize().y )
	, m_world( context, m_bounds, 100 )
	, m_board( m_world, m_playerIsWhite )
{
	const auto& font = context.fontManager->LoadResource( Reflex::ResourceID::ArialFont, "Data/Fonts/arial.ttf" );

	m_turnText[0] = sf::Text( "Your turn", font, 40U );
	m_turnText[1] = sf::Text( "Computer's turn", font, 40U );

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

	if( m_board.m_selectedMarble )
	{
		const auto mousePosition = Reflex::ToVector2f( sf::Mouse::getPosition( *GetContext().window ) );
		m_board.m_selectedMarble->GetTransform()->setPosition( mousePosition );
	}

	return true;
}

bool PentagoGameState::ProcessEvent( const sf::Event& event )
{
	m_world.ProcessEvent( event );

	return true;
}