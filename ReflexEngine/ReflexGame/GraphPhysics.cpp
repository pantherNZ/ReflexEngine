#include "GraphPhysics.h"
#include "..\ReflexEngine\World.h"
#include "GraphNode.h"
#include "..\ReflexEngine\TransformComponent.h"
#include "..\ReflexEngine\Utility.h"
#include "GraphRenderer.h"

using namespace Reflex::Components;

void GraphPhysics::RegisterComponents()
{
	RequiresComponent( GraphNode );
	RequiresComponent( TransformComponent );
}

void GraphPhysics::Update( const sf::Time deltaTime )
{
	const auto k = std::sqrt( ( m_bounds.width * m_bounds.height ) / m_components.size() );
	const float mult = 2.0f, gravity = 5.0f;
	static float temperature = 300.0f;

	if( temperature )
	{

		for( auto& component : m_components )
		{
			auto transform = GetSystemComponent< TransformComponent >( component );
			auto node = GetSystemComponent< GraphNode >( component );

			temperature = std::max( 0.0f, temperature - deltaTime.asSeconds() / 10.0f );
			sf::Vector2f force( 0.0f, 0.0f );

			const auto pos = transform->getPosition();
			const int i = 5;
			const auto other = transform->GetObject();

			// Repulsive forces
			//GetWorld().GetTileMap().ForEachNearby( transform->GetObject(), [&]( const ObjectHandle& obj )
			//{
			//	auto otherTransform = obj->GetComponent< Reflex::Components::TransformComponent >();
			//	const auto test = otherTransform->getPosition();
			//	auto springDirection = pos - test;
			//	const float dist = Reflex::GetMagnitude( springDirection );
			//	springDirection /= dist;
			//	force += springDirection * ( k * k ) / dist;
			//	force -= springDirection * ( dist * dist ) / k;
			//} );

			for( auto& otherComponent : m_components )
			{
				if( component != otherComponent )
				{
					auto otherTransform = GetSystemComponent< TransformComponent >( otherComponent );
					auto springDirection = transform->getPosition() - otherTransform->getPosition();
					const float dist = Reflex::GetMagnitude( springDirection );
					springDirection /= dist;
					force += 1.0f * springDirection * ( k * k ) / dist;
					force -= 1.0f * springDirection * ( dist * dist ) / k;
				}
			}

			// Attractive forces
			for( auto connection : node->m_connections )
			{
				auto springDirection = connection->getPosition() - transform->getPosition();
				const float dist = Reflex::GetMagnitude( springDirection );
				springDirection /= dist;
				force += 1.0f * springDirection * ( dist * dist ) / ( k + node->m_connections.size() * 50 );
			}

			force += /*Reflex::Normalise(*/ ( sf::Vector2f( m_bounds.width / 2.0f, m_bounds.height / 2.0f ) - transform->getPosition() ) * gravity * ( node->m_connections.size() > 0 ? node->m_connections.size() * 1.0f : 1.0f );
			const float mag = Reflex::GetMagnitude( force );
			if( mag >= temperature )
				force *= ( temperature / mag );
			transform->move( force * deltaTime.asSeconds() * mult );
			transform->setPosition( std::min( m_bounds.width, std::max( m_bounds.left, transform->getPosition().x ) ), std::min( m_bounds.height, std::max( m_bounds.top, transform->getPosition().y ) ) );

			for( auto index : node->m_vertexArrayIndices )
			{
				auto* system = GetWorld().GetSystem< GraphRenderer >();
				system->m_connections[index].position = transform->getPosition();
			}
		}
	}
}
