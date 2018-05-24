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
	world.GetContext().textureManager->LoadResource( Reflex::ResourceID::Egg1, "Data/Textures/GoldEgg.png" );
	world.GetContext().textureManager->LoadResource( Reflex::ResourceID::Egg2, "Data/Textures/BlackEgg.png" );

	// Board size will be 60% of the world boundary (screen height)
	const auto centre = sf::Vector2f( m_world.GetBounds().left + m_world.GetBounds().width / 2.0f, m_world.GetBounds().top + m_world.GetBounds().height / 2.0f );
	m_boardBounds.width = m_boardBounds.height = m_world.GetBounds().height * 0.6f;
	m_boardBounds.left = centre.x - m_boardBounds.width / 2.0f;
	m_boardBounds.top = centre.y - m_boardBounds.height / 2.0f;
	m_marbleSize = m_boardBounds.width / 5.0f;
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
			cornerVisual->GetSprite().setScale( sf::Vector2f( ( m_boardBounds.width / 2.0f - 10.0f ) / cornerVisual->GetSprite().getTextureRect().width, ( m_boardBounds.height / 2.0f - 10.0f ) / cornerVisual->GetSprite().getTextureRect().height ) );
		}
	}

	auto playerMarble = m_world.CreateObject( centre + sf::Vector2f( boardSize / 2.0f + 50.0f, 0.0f ) );
	auto circle = playerMarble->AddComponent< Reflex::Components::SFMLObject >( sf::CircleShape( m_marbleSize / 4.0f ) );
	circle->GetCircleShape().setFillColor( playerIsWhite ? sf::Color::White : sf::Color::Black );
	
	auto interactable = playerMarble->AddComponent< Reflex::Components::Interactable >();
	interactable->m_selectionIsToggle = false;
	interactable->m_selectedCallback = [=]( const InteractableHandle& interactable )
	{
		circle->GetCircleShape().setFillColor( sf::Color::Blue );

		if( !m_selectedMarble )
		{
			m_selectedMarble = m_world.CreateObject();
			m_selectedMarble->CopyComponentsFrom< Reflex::Components::Transform, Reflex::Components::SFMLObject >( playerMarble );
		}
	};

	interactable->m_deselectedCallback = [=]( const InteractableHandle& interactable )
	{
		circle->GetCircleShape().setFillColor( playerIsWhite ? sf::Color::White : sf::Color::Black );

		if( m_selectedMarble )
		{
			m_selectedMarble->Destroy();
			m_selectedMarble = ObjectHandle::null;
		}
	};

	// Testing the board
	//for( unsigned y = 0U; y < 6; ++y )
	//	for( unsigned x = 0U; x < 6; ++x )
	//		PlaceMarble( false, x, y );
}
