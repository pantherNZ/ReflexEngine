#include "SpacialHashMapDemo.h"
#include "..\ReflexEngine\SFMLObjectComponent.h"
#include "..\ReflexEngine\TransformComponent.h"

#include <SFML\Window\Mouse.hpp>

SpacialHashMapDemo::SpacialHashMapDemo( Reflex::Core::StateManager& stateManager, Reflex::Core::Context context )
	: State( stateManager, context )
	, m_bounds( 0.0f, 0.0f, ( float )context.window->getSize().x, ( float )context.window->getSize().y )
	, m_world( context, m_bounds, 250U )
	, m_objectCount( 500 )
{
	for( unsigned i = 0U; i < m_objectCount; ++i )
	{
		auto newObject = m_world.CreateObject( sf::Vector2f( m_bounds.left + Reflex::RandomFloat() * m_bounds.width, m_bounds.top + Reflex::RandomFloat() * m_bounds.height ) );
		auto& shape = newObject->AddComponent< Reflex::Components::SFMLObject >( sf::CircleShape( 10.0f ) )->GetCircleShape();
		shape.setOutlineThickness( 2.0f );
		shape.setFillColor( sf::Color::Transparent );
		shape.setOutlineColor( sf::Color::Blue );
	}
}

void SpacialHashMapDemo::Render()
{
	PROFILE;
	m_world.Render();
}

bool SpacialHashMapDemo::Update( const float deltaTime )
{
	PROFILE;

	m_world.Update( deltaTime );

	m_world.ForEachObject( [&]( Object* obj )
	{
		auto& transform = obj->GetComponent< Reflex::Components::Transform >();
		transform->move( 0.0f, 5.0f * deltaTime );
		auto pos = transform->GetWorldPosition();
		pos.y += 250.0f * deltaTime;
		pos.y = ( pos.y >= m_bounds.height ? 0.0f : pos.y );
		transform->setPosition( pos );

		auto& shape = obj->GetComponent< Reflex::Components::SFMLObject >()->GetCircleShape();
		shape.setOutlineColor( sf::Color::Blue );
	} );

	const auto mouse_position = sf::Mouse::getPosition( *GetContext().window );
	m_world.GetTileMap().ForEachNearby( Reflex::Vector2iToVector2f( mouse_position ), [&]( ObjectHandle obj )
	{
		obj->GetComponent< Reflex::Components::SFMLObject >()->GetCircleShape().setOutlineColor( sf::Color::Red );
	} );

	return true;
}

bool SpacialHashMapDemo::ProcessEvent( const sf::Event& event )
{
	return true;
}
