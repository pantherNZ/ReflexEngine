#include "PentagoGameState.h"
#include "Resources.h"
#include "MarbleComponent.h"
#include "..\ReflexEngine\Logging.h"

unsigned PentagoGameState::m_AIDifficulty = 3U;

PentagoGameState::PentagoGameState( StateManager& stateManager, Context context )
	: State( stateManager, context )
	, m_bounds( 0.0f, 0.0f, ( float )context.window->getSize().x, ( float )context.window->getSize().y )
	, m_world( context, m_bounds, 100 )
	, m_board( m_world, *this, m_playerIsWhite )
{
//	const auto& font = context.fontManager->GetResource( Reflex::ResourceID::ArialFont );
	const auto& font = context.fontManager->LoadResource( Reflex::ResourceID::ArialFont, "Data/Fonts/arial.ttf" );

	//const auto& endGameScreen = m_world.GetContext().textureManager->LoadResource( Reflex::ResourceID::EndScreen1, "Data/Textures/EndScreen1.png" );
	//m_gameBoard = m_world.CreateObject( centre );
	//auto grid = m_gameBoard->AddComponent< Reflex::Components::Grid >( 2U, 2U, boardSize / 2.0f, boardSize / 2.0f );
	//m_gameBoard->AddComponent< Reflex::Components::SFMLObject >( sf::Sprite( background ) );

	m_text[0] = sf::Text( "Computer's turn", font, 40U );
	m_text[1] = sf::Text( "Your turn", font, 40U );
	m_text[2] = sf::Text( "Game Over - You lost!", font, 40U );
	m_text[3] = sf::Text( "Well done - You won!", font, 40U );

	for( unsigned i = 0U; i < 4; ++i )
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
		const auto window = GetContext().window;
		const auto mousePosition = window->mapPixelToCoords( sf::Mouse::getPosition( *window ) );
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
	else if( m_board.m_boardState == GameState::AITurn )
	{
		if( m_AIThread && m_AIThreadFinished )
		{
			Reflex::LOG_INFO( "AI move: Corner = " << m_nextAIMove.corner << ", Index = ( " << m_nextAIMove.index.x << ", " << m_nextAIMove.index.y << " )" );

			m_board.PlaceAIMarble( m_nextAIMove.corner, m_nextAIMove.index );

			if( !m_gameOver )
			{
				m_AITimer = 0.5f + Reflex::RandomFloat( 1.0f );
				m_board.m_boardState = GameState::AISpinSelection;
			}
		}
	}

	return true;
}

bool PentagoGameState::ProcessEvent( const sf::Event& event )
{
	m_world.ProcessEvent( event );

	if( event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::Escape )
		RequestStackPush( InGameMenuStateType );

	return true;
}

void PentagoGameState::SetTurn( const bool playerTurn )
{
	m_playerTurn = playerTurn;

	if( !m_playerTurn )
		m_AITimer = 0.1f;
}

void PentagoGameState::GameOver( const bool playerWin )
{
	m_gameOver = true;
}

int ProcessMoves( BoardData& board, const unsigned depth, int alpha, int beta, const BoardType player, Move& bestMove )
{
	int score = 0;
	const auto result = board.CheckWin( true, score );

	if( depth == 0 || result != Empty )
		return score;

	PROFILE;
	auto moveFound = false;
	auto bestScore = -std::numeric_limits< int >::max() * player;
	auto escapeLoop = false;

	for( Corner corner = TopLeft; corner < Corner::NumCorners && !escapeLoop; corner = Corner( corner + 1 ) )
	{
		for( unsigned i = 0U; i < 9 && !escapeLoop; ++i )
		{
			const auto index = sf::Vector2u( i % 3, i / 3 );

			if( board.GetTile( corner, index ) != BoardType::Empty )
				continue;

			// Place player piece
			board.SetTile( corner, index, player );

			bool rotateLeft = true;

			for( Corner cornerRot = None; cornerRot < Corner::NumCorners && !escapeLoop; )
			{
				// Rotate
				if( cornerRot != Corner::None )
					board.RotateCorner( cornerRot, rotateLeft );

				// Score
				Move opponentbestMove;
				const auto newScore = ProcessMoves( board, depth - 1, alpha, beta, BoardType( player * -1 ), opponentbestMove );

				// A - B pruning
				if( player == AIMarble )
					alpha = std::max( alpha, newScore );
				else
					beta = std::min( beta, newScore );

				// New best score
				if( ( player == AIMarble && newScore > bestScore ) || ( player == PlayerMarble && newScore < bestScore ) )
				{
					bestScore = newScore;
					bestMove.corner = corner;
					bestMove.index = index;
					bestMove.rotation = cornerRot;
					bestMove.leftRotation = rotateLeft;
					moveFound = true;
				}

				if( alpha >= beta )
					escapeLoop = true;

				// Undo rotation
				if( cornerRot != Corner::None )
					board.RotateCorner( cornerRot, !rotateLeft );

				// Increment
				if( rotateLeft && cornerRot != Corner::None )
					rotateLeft = false;
				else
				{
					cornerRot = Corner( cornerRot + 1 );
					rotateLeft = true;
				}
			}

			// Undo previous move
			board.SetTile( corner, index, BoardType::Empty );
		}
	}

	assert( moveFound );
	if( !moveFound )
		return 0;

	return bestScore;
}

void ProcessAIThread( void* data )
{
	PentagoGameState* gameState = static_cast< PentagoGameState* >( data );
	int alpha = -std::numeric_limits< int >::max();
	int beta = std::numeric_limits< int >::max();

	auto searchDepth = PentagoGameState::m_AIDifficulty;

	// Easy - chance for 2 search depth
	if( searchDepth == 1 )
	{
		if( Reflex::RandomInt( 100 ) < 15 )
			searchDepth = 2;
	}
	// Medium - chance for 1 & 3 search depth
	else if( searchDepth == 2 )
	{
		if( Reflex::RandomInt( 100 ) < 30 )
			searchDepth += Reflex::RandomInt( -1, 1 );
	}

	const auto score = ProcessMoves( gameState->m_AIBoard, searchDepth, alpha, beta, BoardType::AIMarble, gameState->m_nextAIMove );
	Reflex::LOG_INFO( "Thread finished" );
	gameState->m_AIThreadFinished = true;
}

void PentagoGameState::AITurn()
{
	// Regular move
	if( m_board.m_boardState == GameState::AITurn )
	{
		m_AIBoard.data[0] = m_board.m_boardData.data[0];
		m_AIBoard.data[1] = m_board.m_boardData.data[1];

		//m_AIThreadFinished = true;
		//int alpha = -std::numeric_limits< int >::max();
		//int beta = std::numeric_limits< int >::max();
		//ProcessMoves( m_AIBoard, m_AISearchDepth, alpha, beta, BoardType::AIMarble, m_nextAIMove );

		m_AIThreadFinished = false;
		m_AIThread.reset( new sf::Thread( &ProcessAIThread, this ) );
		m_AIThread->launch();
	}
	else
	{
		// Rotation move
		if( m_nextAIMove.rotation != Corner::None )
		{
			m_board.RotateCorner( m_nextAIMove.rotation, m_nextAIMove.leftRotation );
		}
		else
		{
			// Skip move
			m_board.m_boardState = GameState::PlayerTurn;
			SetTurn( true );
		}
	}
}