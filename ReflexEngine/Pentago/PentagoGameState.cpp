#include "PentagoGameState.h"
#include "Resources.h"

PentagoGameState::PentagoGameState( StateManager& stateManager, Context context )
	: State( stateManager, context )
	, m_bounds( 0.0f, 0.0f, ( float )context.window->getSize().x, ( float )context.window->getSize().y )
	, m_world( context, m_bounds, 100 )
	, m_board( m_world, *this, m_playerIsWhite )
{
	const auto& font = context.fontManager->LoadResource( Reflex::ResourceID::ArialFont, "Data/Fonts/arial.ttf" );

	m_text[0] = sf::Text( "Computer's turn", font, 40U );
	m_text[1] = sf::Text( "Your turn", font, 40U );
	m_text[2] = sf::Text( "Game Over - You lost!", font, 40U );
	m_text[3] = sf::Text( "Well done - You won!", font, 40U );

	for( unsigned i = 0U; i < 2; ++i )
	{
		m_text[i].setPosition( sf::Vector2f( m_board.m_boardBounds.left + m_board.m_boardBounds.width / 2.0f, m_board.m_boardBounds.top - 50.0f ) );
		m_text[i].setFillColor( sf::Color::White );
		Reflex::CenterOrigin( m_text[i] );
	}
}

void PentagoGameState::Render()
{
	m_world.Render();

	GetContext().window->draw( m_text[( int )m_playerTurn + ( m_gameOver ? 2 : 0 )] );
}

bool PentagoGameState::Update( const float deltaTime )
{
	m_world.Update( deltaTime );

	if( m_board.m_selectedMarble )
	{ 
		const auto mousePosition = Reflex::ToVector2f( sf::Mouse::getPosition( *GetContext().window ) );
		m_board.m_selectedMarble->GetTransform()->setPosition( mousePosition );
	}

	if( m_AITimer > 0.0f )
	{
		m_AITimer -= deltaTime;

		if( m_AITimer <= 0.0f )
		{
			m_AITimer = 0.0f;
			AITurn();
		}
	}

	return true;
}

bool PentagoGameState::ProcessEvent( const sf::Event& event )
{
	m_world.ProcessEvent( event );

	return true;
}

void PentagoGameState::SetTurn( const bool playerTurn )
{
	m_playerTurn = playerTurn;

	if( !m_playerTurn )
		m_AITimer = 1.0f + Reflex::RandomFloat( 3.0f );
}

void PentagoGameState::GameOver( const bool playerWin )
{
	m_gameOver = true;
}

void PentagoGameState::AITurn()
{
	if( m_board.m_boardState == GameState::AITurn )
	{
		std::vector< sf::Vector2u > openSpots( 10 );

		m_board.ForEachSlot( [&openSpots]( const Reflex::Core::ObjectHandle& obj, const sf::Vector2u index )
		{
			if( !obj )
				openSpots.emplace_back( index );
		} );

		m_board.PlaceAIMarble( openSpots[Reflex::RandomInt( openSpots.size() - 1 )] );

		if( !m_gameOver )
		{
			m_AITimer = 0.5f + Reflex::RandomFloat( 1.0f );
			m_board.m_boardState = GameState::AICornerSpinning;
		}
	}
	else
	{
		m_board.m_boardState = GameState::PlayerTurn;
		m_board.RotateCorner( Reflex::RandomInt( 1U ), Reflex::RandomInt( 1U ), Reflex::RandomBool() );
	}
}