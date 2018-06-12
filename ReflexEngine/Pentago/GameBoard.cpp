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
	//auto circle = playerMarble->AddComponent< Reflex::Components::SFMLObject >( sf::CircleShape( m_marbleSize / 2.0f ) );
	auto circle = m_playerMarble->AddComponent< Reflex::Components::SFMLObject >( sf::Sprite( playerIsWhite ? egg1 : egg2 ) );
	Reflex::ScaleTo( circle->GetSprite(), sf::Vector2f( m_marbleSize, m_marbleSize ) );

	auto interactable = m_playerMarble->AddComponent< Reflex::Components::Interactable >();
	interactable->selectionIsToggle = false;
	interactable->selectedCallback = [this]( const InteractableHandle& interactable )
	{
		if( !m_selectedMarble && m_boardState == GameState::PlayerTurn )
		{
			m_selectedMarble = m_world.CreateObject();
			m_selectedMarble->CopyComponentsFrom< Reflex::Components::Transform, Reflex::Components::SFMLObject >( m_playerMarble );
			m_selectedMarble->AddComponent< Marble >( true );
		}
	};

	interactable->deselectedCallback = [this, grid]( const InteractableHandle& interactable )
	{
		if( m_selectedMarble )
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
						
						if( CheckWin( result.second.x * 3 + result2.second.x, result.second.y * 3 + result2.second.y, true ) )
						{
							m_boardState = GameState::PlayerWin;
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
	m_cornerArrows[4] = m_world.CreateObject( centre + sf::Vector2f( boardSize / 2.0f + arrowOffset2, boardSize / 2.0f - arrowOffset ), 20.0f );
	m_cornerArrows[5] = m_world.CreateObject( centre + sf::Vector2f( boardSize / 2.0f - arrowOffset, boardSize / 2.0f + arrowOffset2 ), 250.0f );
	m_cornerArrows[6] = m_world.CreateObject( centre + sf::Vector2f( -boardSize / 2.0f + arrowOffset, boardSize / 2.0f + arrowOffset2 ), 110.0f );
	m_cornerArrows[7] = m_world.CreateObject( centre + sf::Vector2f( -boardSize / 2.0f - arrowOffset2, boardSize / 2.0f - arrowOffset ), -20.0f );

	// Button functionality
	for (unsigned i = 0U; i < 8; i++)
	{
		const auto arrowObj = m_cornerArrows[i]->AddComponent< Reflex::Components::SFMLObject >( sf::Sprite( i % 2 ? arrowRight : arrowLeft ) );
		Reflex::ScaleTo( arrowObj->GetSprite(), sf::Vector2f( arrowSize, arrowSize ) );
		arrowObj->GetSprite().setColor( sf::Color::Transparent );

		auto interactable = m_cornerArrows[i]->AddComponent< Reflex::Components::Interactable >();
		interactable->selectionIsToggle = false;

		interactable->selectedCallback = [this, i]( const InteractableHandle& interactable )
		{
			if( m_boardState == GameState::PlayerSpinSelection )
			{
				ToggleArrows( false );
				const auto index = i / 2U;
				RotateCorner( index % 2, index / 2U, i % 2 == 0 );
			}
		};

		interactable->gainedFocusCallback = [this, arrowObj, arrowSize]( const InteractableHandle& interactable )
		{
			Reflex::ScaleTo( arrowObj->GetSprite(), sf::Vector2f( arrowSize * 1.5f, arrowSize * 1.5f ) );
		};

		interactable->lostFocusCallback = [this, arrowObj, arrowSize]( const InteractableHandle& interactable )
		{
			Reflex::ScaleTo( arrowObj->GetSprite(), sf::Vector2f( arrowSize, arrowSize ) );
		};
	}

	// Testing the board
	//for( unsigned y = 0U; y < 6; ++y )
	//	for( unsigned x = 0U; x < 6; ++x )
	//		PlaceMarble( x, y );
}

void GameBoard::PlaceAIMarble( const sf::Vector2u index )
{
	auto newObj = m_world.CreateObject();
	newObj->CopyComponentsFrom< Reflex::Components::Transform, Reflex::Components::SFMLObject >( m_playerMarble );
	newObj->GetComponent< Reflex::Components::SFMLObject >()->GetSprite().setTexture( m_world.GetContext().textureManager->GetResource( Reflex::ResourceID::Egg2 ) );
	newObj->AddComponent< Marble >( false );
	auto corner = m_gameBoard->GetComponent< Reflex::Components::Grid >()->GetCell( index.x / 3, index.y / 3 );
	auto cornerGrid = corner->GetComponent< Reflex::Components::Grid >();
	cornerGrid->AddToGrid( newObj, index.x % 3, index.y % 3 );

	if( CheckWin( index.x, index.y, false ) )
	{
		m_boardState = GameState::AIWin;
		m_gameState.GameOver( false );
	}
}

void GameBoard::ToggleArrows( const bool show )
{
	const auto colour = show ? sf::Color::White : sf::Color::Transparent;
	for( unsigned i = 0U; i < 8; i++ )
	{
		m_cornerArrows[i]->GetComponent< Reflex::Components::SFMLObject >()->GetSprite().setColor( colour );
		m_cornerArrows[i]->GetComponent< Reflex::Components::Interactable >()->isEnabled = show;
	}
}

void GameBoard::RotateCorner( const unsigned x, const unsigned y, const bool rotateLeft )
{
	m_boardState = GameState::PlayerCornerSpinning;

	auto corner = m_gameBoard->GetComponent< Reflex::Components::Grid >()->GetCell( x, y );
	corner->GetComponent< Reflex::Components::Transform >()->RotateForDuration( 90.0f * ( rotateLeft ? -1.0f : 1.0f ), 2.0f,
	[this]( const TransformHandle& transform )
	{
		m_boardState = m_boardState == GameState::PlayerCornerSpinning ? GameState::AITurn : GameState::PlayerTurn;
		m_gameState.SetTurn( m_boardState == GameState::PlayerTurn );
	} );
}

bool GameBoard::CheckWin( const int locX, const int locY, const bool isPlayer )
{
	int counters[4] = { 0, 0, 0, 0 };

	const auto CheckGrid = [&]( unsigned x, unsigned y ) -> bool
	{
		auto corner = m_gameBoard->GetComponent< Reflex::Components::Grid >()->GetCell( x / 3, y / 3 );
		auto cell = corner->GetComponent< Reflex::Components::Grid >()->GetCell( x % 3, y % 3 );
		return cell && cell->GetComponent< Marble >()->isPlayer == isPlayer;
	};

	const auto Check = [&]( unsigned index, unsigned x, unsigned y ) -> bool
	{
		counters[index] = CheckGrid( x, y ) ? counters[index] + 1 : 0U;
		return counters[index] >= 5;
	};

	for( int offset = 0U; offset < 6; ++offset )
	{
		bool win = false;
		win |= Check( 0, offset, locY );
		win |= Check( 1, locX, offset );

		if( abs( locX - locY ) <= 1 )
			win |= Check( 2, std::min( 5, std::max( 0, locX - locY ) + offset ), std::min( 5, std::max( 0, locY - locX ) + offset ) );

		if( abs( locX - locY ) >= 4 )
			win |= Check( 3, std::max( 0, abs( locX + locY - std::min( 5, locX + locY ) ) ), std::max( 0, std::min( 5, locX + locY ) - offset ) );

		if( win )
			return true;
	}

	return false;
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
