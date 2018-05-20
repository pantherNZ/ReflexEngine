#include "GameBoard.h"

#include "..\ReflexEngine\SFMLObjectComponent.h"
#include "..\ReflexEngine\GRIDComponent.h"
#include "MarbleComponent.h"
#include "..\ReflexEngine\InteractableComponent.h"
#include "Resources.h"

GameBoard::GameBoard( World& world, const bool playerIsWhite )
	: m_world( world )
	, m_bounds( Reflex::ToAABB( m_world.GetBounds() ) )
{
	const auto& background = world.GetContext().textureManager->LoadResource( Reflex::ResourceID::BackgroundTexture, "Data/Textures/BackgroundPlaceholder.png" );
	const auto& plate = world.GetContext().textureManager->LoadResource( Reflex::ResourceID::BoardTexture, "Data/Textures/MovingPlateUpdate.png" );
	world.GetContext().textureManager->LoadResource( Reflex::ResourceID::Egg1, "Data/Textures/GoldEgg.png" );
	world.GetContext().textureManager->LoadResource( Reflex::ResourceID::Egg2, "Data/Textures/BlackEgg.png" );

	// Board size will be 60% of the world boundary (screen height)
	m_bounds.halfSize = sf::Vector2f( m_bounds.halfSize.y, m_bounds.halfSize.y ) * 0.6f;
	m_marbleSize = m_bounds.halfSize.x / 5.0f;

	auto gameBoard = m_world.CreateObject( m_bounds.centre );
	auto grid = gameBoard->AddComponent< Reflex::Components::Grid >( sf::Vector2u( 2U, 2U ), m_bounds.halfSize );
	gameBoard->AddComponent< Reflex::Components::SFMLObject >( sf::Sprite( background ) );

	for( unsigned y = 0U; y < 2; ++y )
	{
		for( unsigned x = 0U; x < 2; ++x )
		{
			auto cornerObject = m_world.CreateObject( false );

			cornerObject->AddComponent< Reflex::Components::Grid >( sf::Vector2u( 3U, 3U ), m_bounds.halfSize / 3.0f );
			grid->AddToGrid( cornerObject, x, y );

			//auto cornerVisual = cornerObject->AddComponent< Reflex::Components::SFMLObject >( sf::RectangleShape( m_bounds.halfSize - sf::Vector2f( 10.0f, 10.0f ) ) );
			auto cornerVisual = cornerObject->AddComponent< Reflex::Components::SFMLObject >( sf::Sprite( plate ) );
			//cornerVisual->GetSprite().setTextureRect( Reflex::ToIntRect( Reflex::AABB( m_bounds.centre, m_bounds.halfSize - sf::Vector2f( 10.0f, 10.0f ) ) ) );
			//cornerVisual->GetRectangleShape().setFillColor( sf::Color::Green );
		}
	}

	auto playerMarble = m_world.CreateObject( m_bounds.centre + sf::Vector2f( m_bounds.halfSize.x + 50.0f, 0.0f ) );
	auto circle = playerMarble->AddComponent< Reflex::Components::SFMLObject >( sf::CircleShape( m_marbleSize / 2.0f ) );
	circle->GetCircleShape().setFillColor( playerIsWhite ? sf::Color::White : sf::Color::Black );
	
	auto interactable = playerMarble->AddComponent< Reflex::Components::Interactable >();
	interactable->m_gainedFocusCallback = [=]( const InteractableHandle& interactable )
	{
		circle->GetCircleShape().setFillColor( sf::Color::Blue );
		//if( !m_selectedMarble )
		//{
		//	m_selectedMarble = m_world.CreateObject();
		//	m_selectedMarble->CopyComponentsFrom< Reflex::Components::Transform, Reflex::Components::SFMLObject >( playerMarble );
		//}
	};

	interactable->m_lostFocusCallback = [=]( const InteractableHandle& interactable )
	{
		circle->GetCircleShape().setFillColor( playerIsWhite ? sf::Color::White : sf::Color::Black );
	};

	// Testing the board
	//for( unsigned y = 0U; y < 6; ++y )
	//	for( unsigned x = 0U; x < 6; ++x )
	//		PlaceMarble( false, x, y );
}
/*
void GameBoard::PlaceMarble( const bool black, const sf::Vector2f& position )
{
	ObjectHandle bottom = m_tree.FindBottomMost( position );

	if( !bottom )
		return;

	auto grid = bottom->GetComponent< Reflex::Components::Grid >();

	if( !grid )
		return;

	PlaceMarble( black, grid->GetCellIndex( position ).second );
}

void GameBoard::PlaceMarble( const bool black, const unsigned x, const unsigned y )
{
	PlaceMarble( black, sf::Vector2u( x, y ) );
}

void GameBoard::PlaceMarble( const bool black, const sf::Vector2u& index )
{
	if( index.x >= 6 || index.y >= 6 )
		return;

	auto root = m_world.GetSceneObject();
	auto grid = root->GetComponent< Reflex::Components::Grid >();
	auto cornerObject = grid->GetCell( index / 3U );
	auto cornerGrid = cornerObject->GetComponent< Reflex::Components::Grid >();

	auto newMarble = m_world.CreateObject( false );
	auto component = newMarble->AddComponent< Reflex::Components::SFMLObject >( sf::CircleShape( m_marbleSize / 2.0f ) );
	component->GetRectangleShape().setFillColor( black ? sf::Color::Black : sf::Color::White );

	const auto transform = newMarble->GetComponent< Reflex::Components::Transform >();
	m_tree.Insert( newMarble, Reflex::AABB( transform->GetWorldPosition(), sf::Vector2f( m_marbleSize / 2.0f, m_marbleSize / 2.0f ) ) );
	cornerGrid->AddToGrid( newMarble, sf::Vector2u( index.x % 3U, index.y % 3 ) );
}

ObjectHandle GameBoard::GetMarble( const sf::Vector2f& position ) const
{
	ObjectHandle bottom = m_tree.FindBottomMost( position );

	if( !bottom )
		return ObjectHandle::null;

	if( bottom->HasComponent< Reflex::Components::Grid >() )
		return ObjectHandle::null;

	return bottom;
}

ObjectHandle GameBoard::GetMarble( const unsigned x, const unsigned y ) const
{
	return GetMarble( sf::Vector2u( x, y ) );
}

ObjectHandle GameBoard::GetMarble( const sf::Vector2u& index ) const
{
	if( index.x >= 6 || index.y >= 6 )
		return ObjectHandle::null;

	auto root = m_world.GetSceneObject();
	auto grid = root->GetComponent< Reflex::Components::Grid >();
	auto cornerObject = grid->GetCell( index / 3U );
	auto cornerGrid = cornerObject->GetComponent< Reflex::Components::Grid >();
	return cornerGrid->GetCell( index );
}*/