#include "GameBoard.h"

#include "..\ReflexEngine\SFMLObjectComponent.h"
#include "..\ReflexEngine\GRIDComponent.h"
#include "..\ReflexEngine\InteractableComponent.h"
#include "Resources.h"
#include "MarbleComponent.h"
#include "PentagoGameState.h"

GameBoard::GameBoard( World& world, PentagoGameState& gameState, const bool playerIsWhite )
	: m_world( world )
	, m_gameState( gameState )
{
	const auto& background = world.GetContext().textureManager->LoadResource( Reflex::ResourceID::BackgroundTexture, "Data/Textures/Background.png" );
	const auto& plate = world.GetContext().textureManager->LoadResource( Reflex::ResourceID::BoardTexture, "Data/Textures/MovingPlate.png" );
	const auto& egg1 = world.GetContext().textureManager->LoadResource( Reflex::ResourceID::Egg1, "Data/Textures/Egg1.png" );
	const auto& egg2 = world.GetContext().textureManager->LoadResource( Reflex::ResourceID::Egg2, "Data/Textures/Egg2.png" );
	const auto& arrowLeft = world.GetContext().textureManager->LoadResource( Reflex::ResourceID::ArrowLeft, "Data/Textures/ArrowLeft.png" );
	const auto& arrowRight = world.GetContext().textureManager->LoadResource( Reflex::ResourceID::ArrowRight, "Data/Textures/ArrowRight.png" );
	const auto& skipButton = world.GetContext().textureManager->LoadResource( Reflex::ResourceID::SkipButton, "Data/Textures/SkipButton.png" );

	// Board size will be 60% of the world boundary (screen height)
	const auto centre = sf::Vector2f( m_world.GetBounds().left + m_world.GetBounds().width / 2.0f, m_world.GetBounds().top + m_world.GetBounds().height / 2.0f );
	m_boardBounds.width = m_boardBounds.height = m_world.GetBounds().height * 0.6f;
	m_boardBounds.left = centre.x - m_boardBounds.width / 2.0f;
	m_boardBounds.top = centre.y - m_boardBounds.height / 2.0f;
	m_marbleSize = m_boardBounds.width / 9.5f;
	const float boardSize = m_boardBounds.width;

	m_gameBoard = m_world.CreateObject( centre );
	auto grid = m_gameBoard->AddComponent< Reflex::Components::Grid >( 2U, 2U, boardSize / 2.0f, boardSize / 2.0f );
	m_gameBoard->AddComponent< Reflex::Components::SFMLObject >( sf::Sprite( background ) );

	for( unsigned y = 0U; y < 2; ++y )
	{
		for( unsigned x = 0U; x < 2; ++x )
		{
			auto cornerObject = m_world.CreateObject( false );

			cornerObject->AddComponent< Reflex::Components::Grid >( 3U, 3U, boardSize / 6.0f, boardSize / 6.0f );
			grid->AddToGrid( cornerObject, x, y );

			auto cornerVisual = cornerObject->AddComponent< Reflex::Components::SFMLObject >( sf::Sprite( plate ) );
			Reflex::ScaleTo( cornerVisual->GetSprite(), sf::Vector2f( m_boardBounds.width / 2.0f - 10.0f, m_boardBounds.height / 2.0f - 10.0f ) );
		}
	}

	m_playerMarble = m_world.CreateObject( centre + sf::Vector2f( boardSize / 2.0f + 50.0f, 0.0f ) );
	m_playerMarble->GetTransform()->SetLayer( 5U );
	//auto circle = playerMarble->AddComponent< Reflex::Components::SFMLObject >( sf::CircleShape( m_marbleSize / 2.0f ) );
	auto circle = m_playerMarble->AddComponent< Reflex::Components::SFMLObject >( sf::Sprite( playerIsWhite ? egg1 : egg2 ) );
	Reflex::ScaleTo( circle->GetSprite(), sf::Vector2f( m_marbleSize, m_marbleSize ) );

	auto interactable = m_playerMarble->AddComponent< Reflex::Components::Interactable >();
	interactable->selectionIsToggle = false;
	interactable->selectionChangedCallback = [this, grid]( const InteractableHandle& interactable, const bool selected )
	{
		if( selected )
		{
			if( !m_selectedMarble && m_boardState == GameState::PlayerTurn )
			{
				m_selectedMarble = m_world.CreateObject();
				m_selectedMarble->CopyComponentsFrom< Reflex::Components::Transform, Reflex::Components::SFMLObject >( m_playerMarble );
				m_selectedMarble->AddComponent< Marble >( true );
				m_selectedMarble->GetTransform()->SetZOrder( m_playerMarble->GetTransform()->GetZOrder() + 1U );
			}
		}
		else if( m_selectedMarble )
		{
			const auto mousePosition = m_selectedMarble->GetTransform()->getPosition();
			auto result = grid->GetCellIndex( mousePosition );

			if( result.first )
			{
				auto corner = grid->GetCell( result.second );
				auto cornerGrid = corner->GetComponent< Reflex::Components::Grid >();
				const auto result2 = cornerGrid->GetCellIndex( mousePosition );

				if( result2.first )
				{
					auto object = cornerGrid->GetCell( result2.second );

					if( !object )
					{
						cornerGrid->AddToGrid( m_selectedMarble, result2.second );
						m_selectedMarble = ObjectHandle::null;
						const auto finalIndex = cornerGrid->ConvertCellIndex( result2.second, false ).second;
						const auto cornerIndex = result.second.y * 2 + result.second.x;
						m_boardData.SetTile( Corner( cornerIndex ), finalIndex, BoardType::PlayerMarble );
						const auto result3 = CheckWin();

						Reflex::LOG_INFO( "Player move: Corner = " << cornerIndex << ", Index = ( " << finalIndex.x << ", " << finalIndex.y << " )" );
						m_boardData.PrintBoard();

						if( result3 == GameState::PlayerWin )
						{
							m_boardState = result3;
							m_gameState.GameOver( true );
						}
						else
						{
							ToggleArrows( true );
							m_boardState = GameState::PlayerSpinSelection;
						}

						return;
					}
				}
			}

			m_selectedMarble->Destroy();
			m_selectedMarble = ObjectHandle::null;
		}
	};

	// Arrows
	const auto arrowOffset = boardSize / 4.0f;
	const auto arrowSize = 100.0f;
	const auto arrowOffset2 = 30.0f;

	// Starting from top left
	m_cornerArrows[0] = m_world.CreateObject( centre + sf::Vector2f( -boardSize / 2.0f - arrowOffset2, -boardSize / 2.0f + arrowOffset ), -160.0f );
	m_cornerArrows[1] = m_world.CreateObject( centre + sf::Vector2f( -boardSize / 2.0f + arrowOffset, -boardSize / 2.0f - arrowOffset2 ), 70.0f );
	m_cornerArrows[2] = m_world.CreateObject( centre + sf::Vector2f( boardSize / 2.0f - arrowOffset, -boardSize / 2.0f - arrowOffset2 ), 290.0f );
	m_cornerArrows[3] = m_world.CreateObject( centre + sf::Vector2f( boardSize / 2.0f + arrowOffset2, -boardSize / 2.0f + arrowOffset ), 160.0f );
	m_cornerArrows[4] = m_world.CreateObject( centre + sf::Vector2f( -boardSize / 2.0f + arrowOffset, boardSize / 2.0f + arrowOffset2 ), 110.0f );
	m_cornerArrows[5] = m_world.CreateObject( centre + sf::Vector2f( -boardSize / 2.0f - arrowOffset2, boardSize / 2.0f - arrowOffset ), -20.0f );
	m_cornerArrows[6] = m_world.CreateObject( centre + sf::Vector2f( boardSize / 2.0f + arrowOffset2, boardSize / 2.0f - arrowOffset ), 20.0f );
	m_cornerArrows[7] = m_world.CreateObject( centre + sf::Vector2f( boardSize / 2.0f - arrowOffset, boardSize / 2.0f + arrowOffset2 ), 250.0f );

	// Skip button
	{
		m_skipButton = m_world.CreateObject( centre + sf::Vector2f( 0.0f, boardSize / 2.0f + arrowOffset2 ) );
		const auto sprite = m_skipButton->AddComponent< Reflex::Components::SFMLObject >( sf::Sprite( skipButton ) );
		sprite->GetSprite().setColor( sf::Color::Transparent );
		auto interactable = m_skipButton->AddComponent< Reflex::Components::Interactable >();
		interactable->selectionIsToggle = false;

		interactable->selectionChangedCallback = [this]( const InteractableHandle& interactable, const bool selected )
		{
			if( selected && m_boardState == GameState::PlayerSpinSelection )
			{
				ToggleArrows( false );
				m_boardState = GameState::AITurn;
				m_gameState.SetTurn( false );
				//m_boardState = GameState::PlayerTurn;
				//m_gameState.SetTurn( true );
			}
		};
	}

	// Button functionality
	for( unsigned i = 0U; i < 8; i++ )
	{
		const auto arrowObj = m_cornerArrows[i]->AddComponent< Reflex::Components::SFMLObject >( sf::Sprite( i % 2 ? arrowRight : arrowLeft ) );
		Reflex::ScaleTo( arrowObj->GetSprite(), sf::Vector2f( arrowSize, arrowSize ) );
		arrowObj->GetSprite().setColor( sf::Color::Transparent );

		auto interactable = m_cornerArrows[i]->AddComponent< Reflex::Components::Interactable >();
		interactable->selectionIsToggle = false;

		interactable->selectionChangedCallback = [this, i]( const InteractableHandle& interactable, const bool selected )
		{
			if( selected && m_boardState == GameState::PlayerSpinSelection )
			{
				ToggleArrows( false );
				RotateCorner( Corner( i / 2U ), i % 2 == 0 );
			}
		};

		//interactable->gainedFocusCallback = [this, arrowObj, arrowSize]( const InteractableHandle& interactable )
		//{
		//	//Reflex::ScaleTo( arrowObj->GetSprite(), sf::Vector2f( arrowSize * 1.5f, arrowSize * 1.5f ) );
		//};
	}

	// Testing the board
	//for( unsigned y = 0U; y < 6; ++y )
	//	for( unsigned x = 0U; x < 6; ++x )
	//		PlaceMarble( x, y );
}

void GameBoard::PlaceAIMarble( const Corner corner, const sf::Vector2u& index )
{
	assert( m_boardData.GetTile( corner, index ) == BoardType::Empty );

	auto newObj = m_world.CreateObject();
	newObj->CopyComponentsFrom< Reflex::Components::Transform, Reflex::Components::SFMLObject >( m_playerMarble );
	newObj->GetComponent< Reflex::Components::SFMLObject >()->GetSprite().setTexture( m_world.GetContext().textureManager->GetResource( Reflex::ResourceID::Egg2 ) );
	newObj->AddComponent< Marble >( false );
	auto cornerObj = m_gameBoard->GetComponent< Reflex::Components::Grid >()->GetCell( corner % 2, corner / 2 );
	auto cornerGrid = cornerObj->GetComponent< Reflex::Components::Grid >();
	cornerGrid->AddToGrid( newObj, cornerGrid->ConvertCellIndex( sf::Vector2u( index.x, index.y ), true ).second );

	m_boardData.SetTile( corner, index, BoardType::AIMarble );

	const auto result3 = m_boardData.CheckWin();

	if( result3 == BoardType::AIMarble )
	{
		m_boardState = GameState::AIWin;
		m_gameState.GameOver( false );
	}

	m_boardData.PrintBoard();
}

void GameBoard::ToggleArrows( const bool show )
{
	const auto colour = show ? sf::Color::White : sf::Color::Transparent;
	for( unsigned i = 0U; i < 8; i++ )
	{
		m_cornerArrows[i]->GetComponent< Reflex::Components::SFMLObject >()->GetSprite().setColor( colour );
		m_cornerArrows[i]->GetComponent< Reflex::Components::Interactable >()->isEnabled = show;
	}

	m_skipButton->GetComponent< Reflex::Components::SFMLObject >()->GetSprite().setColor( colour );
	m_skipButton->GetComponent< Reflex::Components::Interactable >()->isEnabled = show;
}

void GameBoard::RotateCorner( const Corner corner, const bool rotateLeft )
{
	const auto rotationFinal = 90.0f * ( rotateLeft ? -1.0f : 1.0f );
	m_boardData.RotateCorner( corner, rotateLeft );
	m_boardState = m_boardState == GameState::PlayerSpinSelection ? GameState::PlayerCornerSpinning : GameState::AICornerSpinning;

	auto cornerObj = m_gameBoard->GetComponent< Reflex::Components::Grid >()->GetCell( corner % 2, corner / 2 );
	cornerObj->GetTransform()->SetZOrder( 10U );

	cornerObj->GetTransform()->RotateForDuration( rotationFinal, 1.0f,
		[this]( const TransformHandle& transform )
	{
		const auto result = CheckWin();
		transform->SetZOrder( 5U );

		if( result != GameState::NumStates )
		{
			m_boardState = result;
			m_gameState.GameOver( m_boardState == GameState::PlayerWin );
		}
		else
		{
			m_boardState = m_boardState == GameState::PlayerCornerSpinning ? GameState::AITurn : GameState::PlayerTurn;//GameState::PlayerTurn; //
			m_gameState.SetTurn( m_boardState == GameState::PlayerTurn );
		}
	} );

	Reflex::LOG_INFO( ( m_boardState == GameState::PlayerCornerSpinning ? "Player " : "AI " ) << "rotate: Corner = " << corner << ", Left = " << ( rotateLeft ? "True" : "False" ) );
	m_boardData.PrintBoard();
}

GameState GameBoard::CheckWin()
{
	const auto result = m_boardData.CheckWin();
	return ( result == PlayerMarble ? GameState::PlayerWin : ( result == AIMarble ? GameState::AIWin : GameState::NumStates ) );
}

void GameBoard::ForEachSlot( std::function< void( const ObjectHandle& obj, const sf::Vector2u index ) > callback )
{
	m_gameBoard->GetComponent< Reflex::Components::Grid >()->ForEachChild(
		[&callback]( const Reflex::Core::ObjectHandle& obj, const sf::Vector2u topIndex )
	{
		obj->GetComponent< Reflex::Components::Grid >()->ForEachChild(
			[&callback, &topIndex]( const Reflex::Core::ObjectHandle& obj, const sf::Vector2u index )
		{
			callback( obj, sf::Vector2u( topIndex.x * 3 + index.x, topIndex.y * 3 + index.y ) );
		} );
	} );
}

void BoardData::Reset()
{
	for( unsigned corner = 0U; corner < 4; ++corner )
		for( unsigned index = 0U; index < 9; ++index )
			SetTile( Corner( corner ), sf::Vector2u( index % 3, index / 3 ), BoardType::Empty );
}

BoardType BoardData::GetTile( const Corner corner, const sf::Vector2u& index ) const
{
	return data[corner][index.y * 3 + index.x];
}

void BoardData::SetTile( const Corner corner, const sf::Vector2u& index, const BoardType player )
{
	data[corner][index.y * 3 + index.x] = player;
}

void BoardData::RotateCorner( const Corner corner, const bool rotateLeft )
{
	PROFILE;
	const auto copy = data[corner];

	if( rotateLeft )
	{
		data[corner][0] = copy[2];
		data[corner][3] = copy[1];
		data[corner][6] = copy[0];
		data[corner][7] = copy[3];
		data[corner][8] = copy[6];
		data[corner][5] = copy[7];
		data[corner][2] = copy[8];
		data[corner][1] = copy[5];
	}
	else
	{
		data[corner][0] = copy[6];
		data[corner][1] = copy[3];
		data[corner][2] = copy[0];
		data[corner][5] = copy[1];
		data[corner][8] = copy[2];
		data[corner][7] = copy[5];
		data[corner][6] = copy[8];
		data[corner][3] = copy[7];
	}
}

BoardType BoardData::CheckWin()
{
	int a = 0;
	return CheckWin( a );
}

BoardType BoardData::CheckWin( int& score )
{
	auto total = 0;
	auto run = 0;

	const auto ScoreRun = [&]()
	{
		score += abs( run ) <= 1 ? run : ( abs( run ) < 5 ? run * run * run : Reflex::Sign( run ) * std::numeric_limits< int >::max() );
		run = 0;
	};

	const auto Get = [&]( const unsigned corner, const unsigned startIndex )
	{
		//const auto result = ( player == BoardType::Empty || player == data[corner][startIndex] ) ? data[corner][startIndex] : BoardType::Empty;
		const auto value = data[corner][startIndex];

		//if( player != BoardType::Empty && result != player && run )
		if( value != Reflex::Sign( run ) && run )
		{
			ScoreRun();
			run = 0;
		}

		run += value;

		return value;
	};

	const auto Check = [&]( int value, bool first = false ) -> BoardType
	{
		const auto result = ( value == -5 ? PlayerMarble : ( value == 5 ? AIMarble : Empty ) );

		if( !first || result != Empty )
		{
			total = 0;

			if( run )
				ScoreRun();
		}
		else
			total = value;
		
		return result;
	};

	const auto Vertical = [&]( Corner corner, const unsigned startIndex ) -> BoardType
	{
		const auto top = Get( corner, startIndex );
		if( const auto result = Check( top + Get( corner, startIndex + 3 ) + Get( corner, startIndex + 6 ) + Get( corner + 2, startIndex ) + Get( corner + 2, startIndex + 3 ), true ) )
			return result;
		if( const auto result = Check( total - top + Get( corner + 2, startIndex + 6 ) ) )
			return result;
		return BoardType::Empty;
	};

	const auto Horizontal = [&]( Corner corner, const unsigned startIndex ) -> BoardType
	{
		const auto left = Get( corner, startIndex );
		if( const auto result = Check( left + Get( corner, startIndex + 1 ) + Get( corner, startIndex + 2 ) + Get( corner + 1, startIndex ) + Get( corner + 1, startIndex + 1 ), true ) )
			return result;
		if( const auto result = Check( total - left + Get( corner + 1, startIndex + 2 ) ) )
			return result;
		return BoardType::Empty;
	};

	for( unsigned i = 0U; i < 3; ++i )
	{
		if( const auto result = Vertical( TopLeft, i ) )
			return result;
		if( const auto result = Vertical( TopRight, i ) )
			return result;
		if( const auto result = Horizontal( TopLeft, i * 3 ) )
			return result;
		if( const auto result = Horizontal( BottomLeft, i * 3 ) )
			return result;
	}

	// Diagonals
	if( const auto result = Check( Get( TopLeft, 0 ) + Get( TopLeft, 4 ) + Get( TopLeft, 8 ) + Get( BottomRight, 0 ) + Get( BottomRight, 4 ), true ) )
		return result;
	if( const auto result = Check( total - Get( TopLeft, 0 ) + Get( BottomRight, 8 ) ) )
		return result;
	if( const auto result = Check( Get( TopLeft, 1 ) + Get( TopLeft, 5 ) + Get( TopRight, 6 ) + Get( BottomRight, 1 ) + Get( BottomRight, 5 ) ) )
		return result;
	if( const auto result = Check( Get( TopLeft, 3 ) + Get( TopLeft, 7 ) + Get( BottomLeft, 2 ) + Get( BottomRight, 3 ) + Get( BottomRight, 7 ) ) )
		return result;

	if( const auto result = Check( Get( BottomLeft, 6 ) + Get( BottomLeft, 4 ) + Get( BottomLeft, 2 ) + Get( TopRight, 6 ) + Get( TopRight, 4 ), true ) )
		return result;
	if( const auto result = Check( total - Get( BottomLeft, 6 ) + Get( TopRight, 2 ) ) )
		return result;
	if( const auto result = Check( Get( BottomLeft, 3 ) + Get( BottomLeft, 1 ) + Get( TopLeft, 8 ) + Get( TopRight, 3 ) + Get( TopRight, 1 ) ) )
		return result;
	if( const auto result = Check( Get( BottomLeft, 7 ) + Get( BottomLeft, 5 ) + Get( BottomRight, 0 ) + Get( TopRight, 7 ) + Get( TopRight, 5 ) ) )
		return result;

	return BoardType::Empty;
}

/*
BoardType BoardData::CheckWin()
{
	// Check along the main diagonal
	for( unsigned i = 0U; i < 6; ++i )
	{
		const auto result = CheckWinAtIndex( i, i );
		if( result != BoardType::Empty )
			return result;
	}

	// Also check the middle row that is missed
	return CheckWinAtIndex( 5, 0, true );
}

BoardType BoardData::CheckWinAtIndex( const int locX, const int locY, const bool diagonalsOnly, const bool straightsOnly, std::vector< unsigned >& runsPlayer, std::vector< unsigned >& runsAI )
{
	std::array< unsigned, 4 > countersPlayer = { 0, 0, 0, 0 };
	std::array< unsigned, 4 > countersAI = { 0, 0, 0, 0 };

	const auto AppendRuns = [&]( unsigned index )
	{
		if( countersPlayer[index] > 0U )
		{
			runsPlayer.push_back( countersPlayer[index] );
			countersPlayer[index] = 0U;
		}

		if( countersAI[index] > 0U )
		{
			runsAI.push_back( countersAI[index] );
			countersAI[index] = 0U;
		}
	};

	const auto Check = [&]( unsigned index, unsigned x, unsigned y )
	{
		if( data[y][x] == BoardType::Empty )
		{
			AppendRuns( index );
			return BoardType::Empty;
		}

		auto* counter = data[y][x] == BoardType::PlayerMarble ? &countersPlayer : &countersAI;
		( *counter )[index]++;

		if( counter->at( index ) >= 5 )
		{
			AppendRuns( index );
			return data[y][x];
		}

		return BoardType::Empty;
	};

	for( int offset = 0U; offset < 6; ++offset )
	{
		// X axis
		if( !diagonalsOnly )
		{
			if( auto result = Check( 0, offset, locY ) )
				return result;

			// Y axis
			if( auto result = Check( 1, locX, offset ) )
				return result;
		}

		// Top left to bot right diagonal
		if( !straightsOnly )
		{
			if( abs( locX - locY ) <= 1 )
				if( auto result = Check( 2, std::min( 5, std::max( 0, locX - locY ) + offset ), std::min( 5, std::max( 0, locY - locX ) + offset ) ) )
					return result;

			// Bot left to top right diagonal
			if( abs( ( 5 - locX ) - locY ) <= 1 )
				if( auto result = Check( 3, std::min( 5, std::max( 0, abs( locX + locY - std::min( 5, locX + locY ) ) ) + offset ), std::max( 0, std::min( 5, locX + locY ) - offset ) ) )
					return result;
		}
	}

	for( unsigned i = 0U; i < 4; ++i )
		AppendRuns( i );

	return BoardType::Empty;
}

unsigned BoardData::ScoreBoard( const BoardType player )
{
	auto finalScore = 0U;

	std::vector< unsigned > countersAI, countersPlayer;

	const auto Score = [&]( BoardType result )
	{
		if( result == player )
		{
			finalScore += std::numeric_limits< unsigned >::max();
			return;
		}

		auto* counters = player == BoardType::AIMarble ? &countersAI : &countersPlayer;

		for( auto& count : *counters )
			finalScore += ( count * count * count * 10U );

		countersAI.clear();
		countersPlayer.clear();
	};

	// Check along the main diagonal (straights only)
	for( unsigned i = 0U; i < 6; ++i )
		Score( CheckWinAtIndex( i, i, false, true, countersPlayer, countersAI ) );

	// Check along the diagonals
	Score( CheckWinAtIndex( 0, 0, true, false, countersPlayer, countersAI ) );
	Score( CheckWinAtIndex( 0, 1, true, false, countersPlayer, countersAI ) );
	Score( CheckWinAtIndex( 1, 0, true, false, countersPlayer, countersAI ) );

	Score( CheckWinAtIndex( 0, 5, true, false, countersPlayer, countersAI ) );
	Score( CheckWinAtIndex( 0, 4, true, false, countersPlayer, countersAI ) );
	Score( CheckWinAtIndex( 1, 5, true, false, countersPlayer, countersAI ) );

	return finalScore;
}*/

void BoardData::PrintBoard()
{
	std::cout << "----------------\n";

	for( unsigned int y = 0U; y < 6; ++y )
	{
		std::cout << "| ";

		for( unsigned int x = 0U; x < 6; ++x )
		{
			const auto piece = data[( y / 3 ) * 2 + x / 3][( y % 3 ) * 3 + x % 3];
			std::cout << ( piece == BoardType::Empty ? ". " : ( piece == BoardType::PlayerMarble ? "1 " : "2 " ) );
		}

		std::cout << " |\n";
	}

	std::cout << "----------------\n\n";
}