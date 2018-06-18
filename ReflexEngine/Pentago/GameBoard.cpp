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
	interactable->selectedCallback = [this]( const InteractableHandle& interactable )
	{
		if( !m_selectedMarble && m_boardState == GameState::PlayerTurn )
		{
			m_selectedMarble = m_world.CreateObject();
			m_selectedMarble->CopyComponentsFrom< Reflex::Components::Transform, Reflex::Components::SFMLObject >( m_playerMarble );
			m_selectedMarble->AddComponent< Marble >( true );
			m_selectedMarble->GetTransform()->SetZOrder( m_playerMarble->GetTransform()->GetZOrder() + 1U );
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
						
						const auto result3 = CheckWin( result.second.x * 3 + result2.second.x, result.second.y * 3 + result2.second.y );

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

		interactable->selectedCallback = [this]( const InteractableHandle& interactable )
		{
			if( m_boardState == GameState::PlayerSpinSelection )
			{
				ToggleArrows( false );
				m_boardState = GameState::AITurn;
				m_gameState.SetTurn( false );
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
			//Reflex::ScaleTo( arrowObj->GetSprite(), sf::Vector2f( arrowSize * 1.5f, arrowSize * 1.5f ) );
		};

		interactable->lostFocusCallback = [this, arrowObj, arrowSize]( const InteractableHandle& interactable )
		{
			//Reflex::ScaleTo( arrowObj->GetSprite(), sf::Vector2f( arrowSize, arrowSize ) );
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

	const auto result3 = CheckWin( index.x, index.y );

	if( result3 == GameState::AIWin )
	{
		m_boardState = result3;
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

	m_skipButton->GetComponent< Reflex::Components::SFMLObject >()->GetSprite().setColor( colour );
	m_skipButton->GetComponent< Reflex::Components::Interactable >()->isEnabled = show;
}

void GameBoard::RotateCorner( const unsigned x, const unsigned y, const bool rotateLeft )
{
	m_boardState = m_boardState == GameState::PlayerSpinSelection ? GameState::PlayerCornerSpinning : GameState::AICornerSpinning;

	auto corner = m_gameBoard->GetComponent< Reflex::Components::Grid >()->GetCell( x, y );
	corner->GetTransform()->SetZOrder( 10U );

	corner->GetTransform()->RotateForDuration( 90.0f * ( rotateLeft ? -1.0f : 1.0f ), 1.0f,
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
}

GameState GameBoard::CheckWin()
{
	// Check along the main diagonal
	for( unsigned i = 0U; i < 6; ++i )
	{
		const auto result = CheckWin( i, i, false );
		if( result != GameState::NumStates )
			return result;
	}

	// Also check the middle row that is missed
	return CheckWin( 5, 5, false );
}

GameState GameBoard::CheckWin( const int locX, const int locY, const bool rotatedIndex )
{
	sf::Vector2i unrotatedLoc( locX, locY );

	if( rotatedIndex )
	{
		auto corner = m_gameBoard->GetComponent< Reflex::Components::Grid >()->GetCell( locX / 3, locY / 3 );
		auto cornerGrid = corner->GetComponent< Reflex::Components::Grid >();
		unrotatedLoc = Reflex::Vector2uToVector2i( cornerGrid->ConvertCellIndex( sf::Vector2u( locX % 3, locY % 3 ), false ).second );
		unrotatedLoc += sf::Vector2i( locX - ( locX % 3 ), locY - ( locY % 3 ) );
	}

	int countersPlayer[4] = { 0, 0, 0, 0 };
	int countersAI[4] = { 0, 0, 0, 0 };

	const auto Check = [&]( unsigned index, unsigned x, unsigned y ) -> GameState
	{
		auto corner = m_gameBoard->GetComponent< Reflex::Components::Grid >()->GetCell( x / 3, y / 3, true );
		auto cell = corner->GetComponent< Reflex::Components::Grid >()->GetCell( x % 3, y % 3, true );

		if( !cell )
			return GameState::NumStates;

		const auto result = cell->GetComponent< Marble >()->isPlayer ? GameState::PlayerWin : GameState::AIWin;
		auto* counter = result == GameState::PlayerWin ? countersPlayer : countersAI;
		counter[index]++;
		return counter[index] >= 5 ? result : GameState::NumStates;
	};

	for( int offset = 0U; offset < 6; ++offset )
	{
		// X axis
		auto result = Check( 0, offset, unrotatedLoc.y );
		if( result != GameState::NumStates )
			return result;

		// Y axis
		result = Check( 1, unrotatedLoc.x, offset );
		if( result != GameState::NumStates )
			return result;

		// Top left to bot right diagonal
		if( abs( unrotatedLoc.x - unrotatedLoc.y ) <= 1 )
		{
			result = Check( 2, std::min( 5, std::max( 0, unrotatedLoc.x - unrotatedLoc.y ) + offset ), std::min( 5, std::max( 0, unrotatedLoc.y - unrotatedLoc.x ) + offset ) );
			if( result != GameState::NumStates )
				return result;
		}

		// Bot left to top right diagonal
		if( abs( ( 5 - unrotatedLoc.x ) - unrotatedLoc.y ) <= 1 )
		{
			result = Check( 3, std::min( 5, std::max( 0, abs( unrotatedLoc.x + unrotatedLoc.y - std::min( 5, unrotatedLoc.x + unrotatedLoc.y ) ) ) + offset ), std::max( 0, std::min( 5, unrotatedLoc.x + unrotatedLoc.y ) - offset ) );
			if( result != GameState::NumStates )
				return result;
		}
	}

	return GameState::NumStates;
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
