#include "TransformComponent.h"
#include "Object.h"
#include "World.h"

namespace Reflex
{
	namespace Components
	{
		Transform::Transform( const ObjectHandle& object, BaseHandle componentHandle,
			const sf::Vector2f& position /*= sf::Vector2f()*/,
			const float rotation /*= 0.0f*/,
			const sf::Vector2f& scale /*= sf::Vector2f( 1.0f, 1.0f ) */ )
			: Component( object, componentHandle )
			, SceneNode( object )
		{
			sf::Transformable::setPosition( position );
			m_object->GetWorld().GetTileMap().Insert( m_object, AABB( position, sf::Vector2f( 0.0f, 0.0f ) ) );
			setRotation( rotation );
			setScale( scale );
		}

		void Transform::setPosition( float x, float y )
		{
			Transform::setPosition( sf::Vector2f( x, y ) );
		}

		void Transform::setPosition( const sf::Vector2f& position )
		{
			auto& tileMap = m_object->GetWorld().GetTileMap();
			const auto previousID = tileMap.GetID( m_object );

			sf::Transformable::setPosition( position );

			const auto newID = tileMap.GetID( m_object );

			if( previousID != newID )
			{
				tileMap.RemoveByID( m_object, previousID );
				tileMap.Insert( m_object );
			}
		}

		void Transform::move( float offsetX, float offsetY )
		{
			Transform::move( sf::Vector2f( offsetX, offsetY ) );
		}

		void Transform::move( const sf::Vector2f& offset )
		{
			setPosition( getPosition() + offset );
		}
	}
}