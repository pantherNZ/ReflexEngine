#include "TransformComponent.h"
#include "Object.h"
#include "World.h"

namespace Reflex
{
	namespace Components
	{

		TransformComponent::TransformComponent( ObjectHandle object, BaseHandle handle, 
			const sf::Vector2f& position /*= sf::Vector2f()*/, 
			const float rotation /*= 0.0f*/, 
			const sf::Vector2f& scale /*= sf::Vector2f( 1.0f, 1.0f ) */ )
			: Component( object, handle )
		{
			sf::Transformable::setPosition( position );
			m_object->GetWorld().GetTileMap().Insert( m_object, position, position );
			setRotation( rotation );
			setScale( scale );
		}

		void TransformComponent::setPosition( float x, float y )
		{
			TransformComponent::setPosition( sf::Vector2f( x, y ) );
		}

		void TransformComponent::setPosition( const sf::Vector2f& position )
		{
			m_object->GetWorld().GetTileMap().Remove( m_object );
			sf::Transformable::setPosition( position );
			m_object->GetWorld().GetTileMap().Insert( m_object );
		}

		void TransformComponent::move( float offsetX, float offsetY )
		{
			TransformComponent::move( sf::Vector2f( offsetX, offsetY ) );
		}

		void TransformComponent::move( const sf::Vector2f& offset )
		{
			m_object->GetWorld().GetTileMap().Remove( m_object );
			sf::Transformable::move( offset );
			m_object->GetWorld().GetTileMap().Insert( m_object );
		}
	}
}