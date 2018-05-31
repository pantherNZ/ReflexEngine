#include "GameBoard.h"

#include "..\ReflexEngine\SFMLObjectComponent.h"
#include "..\ReflexEngine\GRIDComponent.h"
#include "..\ReflexEngine\InteractableComponent.h"
#include "Resources.h"

GameBoard::GameBoard( World& world, const bool playerIsWhite )
	: m_world( world )
{
	const auto& background = world.GetContext().textureManager->LoadResource( Reflex::ResourceID::BackgroundTexture, "Data/Textures/BackgroundPlaceholder.png" );
	const auto& plate = world.GetContext().textureManager->LoadResource( Reflex::ResourceID::BoardTexture, "Data/Textures/MovingPlateUpdate.png" );
	const auto& eggGold = world.GetContext().textureManager->LoadResource( Reflex::ResourceID::Egg1, "Data/Textures/GoldEgg.png" );
	const auto& eggBlack = world.GetContext().textureManager->LoadResource( Reflex::ResourceID::Egg2, "Data/Textures/BlackEgg.png" );

	// Board size will be 60% of the world boundary (screen height)
	const auto centre = sf::Vector2f( m_world.GetBounds().left + m_world.GetBounds().width / 2.0f, m_world.GetBounds().top + m_world.GetBounds().height / 2.0f );
	m_boardBounds.width = m_boardBounds.height = m_world.GetBounds().height * 0.6f;
	m_boardBounds.left = centre.x - m_boardBounds.width / 2.0f;
	m_boardBounds.top = centre.y - m_boardBounds.height / 2.0f;
	m_marbleSize = m_boardBounds.width / 8.5f;
	const float boardSize = m_boardBounds.width;

	auto gameBoard = m_world.CreateObject( centre );
	auto grid = gameBoard->AddComponent< Reflex::Components::Grid >( 2U, 2U, boardSize / 2.0f, boardSize / 2.0f );
	gameBoard->AddComponent< Reflex::Components::SFMLObject >( sf::Sprite( background ) );

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

	auto playerMarble = m_world.CreateObject( centre + sf::Vector2f( boardSize / 2.0f + 50.0f, 0.0f ) );
	auto circle = playerMarble->AddComponent< Reflex::Components::SFMLObject >( sf::Sprite( playerIsWhite ? eggGold : eggBlack ) );
	Reflex::ScaleTo( circle->GetSprite(), sf::Vector2f( m_marbleSize, m_marbleSize ) );

	auto interactable = playerMarble->AddComponent< Reflex::Components::Interactable >();
	interactable->m_selectionIsToggle = false;
	interactable->m_selectedCallback = [=]( const InteractableHandle& interactable )
	{
		if( !m_selectedMarble )
		{
			m_selectedMarble = m_world.CreateObject();
			m_selectedMarble->CopyComponentsFrom< Reflex::Components::Transform, Reflex::Components::SFMLObject >( playerMarble );
		}
	};

	interactable->m_deselectedCallback = [=]( const InteractableHandle& interactable )
	{
		if( m_selectedMarble )
		{
			const auto mousePosition = Reflex::ToVector2f( sf::Mouse::getPosition( *m_world.GetContext().window ) );
			auto result = grid->GetCellIndex( mousePosition );

			if( result.first )
			{
				auto corner = grid->GetCell( result.second );
				auto cornerGrid = corner->GetComponent< Reflex::Components::Grid >();
				result = cornerGrid->GetCellIndex( mousePosition );

				if( result.first )
				{
					auto object = cornerGrid->GetCell( result.second );

					if( !object )
					{
						cornerGrid->AddToGrid( m_selectedMarble, result.second );
						m_selectedMarble = ObjectHandle::null;
						return;
					}
				}
			}

			m_selectedMarble->Destroy();
			m_selectedMarble = ObjectHandle::null;
		}
	};

	// Testing the board
	for( unsigned y = 0U; y < 6; ++y )
	{
		for( unsigned x = 0U; x < 6; ++x )
		{
			auto newObj = m_world.CreateObject();
			newObj->CopyComponentsFrom< Reflex::Components::Transform, Reflex::Components::SFMLObject >( playerMarble );
			
			auto corner = grid->GetCell( x / 2, y / 2 );
			auto cornerGrid = corner->GetComponent< Reflex::Components::Grid >();
			cornerGrid->AddToGrid( newObj, x % 3, y % 3 );
		}
	}
}
