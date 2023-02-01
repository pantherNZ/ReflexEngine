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
	const auto& displaySprite = world.GetContext().textureManager->LoadResource( Reflex::ResourceID::SideMenuScreen, "Data/Textures/PiecesBoard.png" );

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
	auto* test = m_gameBoard.Get();
	const auto sideScreen = m_world.CreateObject( centre + sf::Vector2f( boardSize / 2.0f + 100.0f, 0.0f ) );
	sideScreen->AddComponent< Reflex::Components::SFMLObject >( sf::Sprite( displaySprite ) );
	const auto sideGrid = sideScreen->AddComponent< Reflex::Components::Grid >( sf::Vector2u( 1, 3 ), sf::Vector2f( 0.0f, displaySprite.getSize().y / 3.0f ) );



	// Player move types
	const auto CreatePlayerMarble = [this, sideGrid, grid]( const sf::Texture& texture, const unsigned index )
	{
		m_playerMarbles[index] = m_world.CreateObject();
		m_playerMarbles[index]->GetTransform()->SetLayer( 5U );
		auto circle = m_playerMarbles[index]->AddComponent< Reflex::Components::SFMLObject >( sf::Sprite( texture ) );
		Reflex::ScaleTo( circle->GetSprite(), sf::Vector2f( m_marbleSize, m_marbleSize ) );
		sideGrid->AddToGrid( m_playerMarbles[index], sf::Vector2u( 0U, index ) );

		auto interactable = m_playerMarbles[index]->AddComponent< Reflex::Components::Interactable >();
		interactable->selectionIsToggle = false;
		interactable->selectionChangedCallback = [this, grid]( const InteractableHandle& interactable, const bool selected )// , const MoveType moveType )
		{
			if( selected )
			{
				if( !m_selectedMarble && m_boardState == GameState::PlayerTurn )
				{
					auto* test = m_gameBoard.Get();
					m_doubleRotate = false;
					m_selectedMarble = m_world.CreateObject();
					m_selectedMarble->CopyComponentsFrom< Reflex::Components::Transform, Reflex::Components::SFMLObject >( m_playerMarbles[0] );// moveType] );
					//m_world.CopyComponentsFrom< Reflex::Components::Transform, Reflex::Components::SFMLObject>(m_selectedMarble, m_playerMarbles[0]);
					m_selectedMarble->AddComponent< Marble >( true );
					m_selectedMarble->GetTransform()->SetLayer( 10U );
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
						//
						//if( moveType == Default || moveType == Spinner )
						//	PlacePlayerMarble( object, cornerGrid, result.second, result2.second );
						//else if( moveType == Destroyer )
						//	RemoveAIMarble( object, cornerGrid, result.second, result2.second );
						//
						//return;
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
		};// [this, grid, index, selectionChangedCallback]( const InteractableHandle& interactable, const bool selected )
		//{
		//	selectionChangedCallback( interactable, selected, MoveType( index ) );
		//};
	};

	CreatePlayerMarble( playerIsWhite ? egg1 : egg2, 0U );

	if( !m_classicMode )
	{
		CreatePlayerMarble( playerIsWhite ? egg1 : egg2, 1U );
		CreatePlayerMarble( playerIsWhite ? egg1 : egg2, 2U );
	}

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

				if( m_doubleRotate )
				{
					RotateCorner( Corner( i / 2U ), i % 2 == 0 );
					m_doubleRotate = false;
				}
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

void GameBoard::PlacePlayerMarble( Reflex::Core::ObjectHandle object, Reflex::Core::GridHandle cornerGrid, const sf::Vector2u& cornerIndex, const sf::Vector2u& index )
{
	if( !object )
	{
		cornerGrid->AddToGrid( m_selectedMarble, index );
		const auto finalIndex = cornerGrid->ConvertCellIndex( index, false ).second;
		const auto cornerType = cornerIndex.y * 2 + cornerIndex.x;
		m_boardData.SetTile( Corner( cornerType ), finalIndex, BoardType::PlayerMarble );
		const auto winResult = CheckWin();

		m_boardData.PrintBoard();

		if( winResult == GameState::PlayerWin )
		{
			m_boardState = winResult;
			m_gameState.GameOver( true );
		}
		else
		{
			ToggleArrows( true );
			m_boardState = GameState::PlayerSpinSelection;
		}

		Reflex::LOG_INFO( "Player move | Type = Default, Corner = " << cornerType << ", Index = ( " << finalIndex.x << ", " << finalIndex.y << " )" );

		m_selectedMarble = ObjectHandle::null;
		return;
	}

	m_selectedMarble->Destroy();
	m_selectedMarble = ObjectHandle::null;
}

void GameBoard::RemoveAIMarble( Reflex::Core::ObjectHandle object, Reflex::Core::GridHandle cornerGrid, const sf::Vector2u& cornerIndex, const sf::Vector2u& index )
{
	if( object && !object->GetComponent< Marble >()->isPlayer )
	{
		cornerGrid->RemoveFromGrid( index );
		const auto finalIndex = cornerGrid->ConvertCellIndex( index, false ).second;
		const auto cornerType = cornerIndex.y * 2 + cornerIndex.x;
		m_boardData.SetTile( Corner( cornerType ), finalIndex, BoardType::Empty );

		m_boardData.PrintBoard();

		ToggleArrows( true );
		m_boardState = GameState::PlayerSpinSelection;

		Reflex::LOG_INFO( "Player move | Type = Removal, Corner = " << cornerType << ", Index = ( " << finalIndex.x << ", " << finalIndex.y << " )" );
		m_doubleRotate = true;
	}

	m_selectedMarble->Destroy();
	m_selectedMarble = ObjectHandle::null;
}

void GameBoard::PlaceAIMarble( const Corner corner, const sf::Vector2u& index )
{
	assert( m_boardData.GetTile( corner, index ) == BoardType::Empty );

	auto newObj = m_world.CreateObject();
	newObj->CopyComponentsFrom< Reflex::Components::Transform, Reflex::Components::SFMLObject >( m_playerMarbles[0] );
	//m_world.CopyComponentsFrom< Reflex::Components::Transform, Reflex::Components::SFMLObject >(newObj, m_playerMarbles[0]);
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