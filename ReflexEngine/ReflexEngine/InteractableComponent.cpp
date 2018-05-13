#include "InteractableComponent.h"
#include "SFMLObjectComponent.h"
#include "Object.h"

namespace Reflex
{
	namespace Components
	{
		Interactable::Interactable( const ObjectHandle& object, const BaseHandle& componentHandle )
			: Component( object, componentHandle )
		{
			const auto sfmlObj = object->GetComponent< SFMLObject >();
			const auto transform = object->GetComponent< Transform >();

			switch( sfmlObj->GetType() )
			{
			case SFMLObjectType::Circle:
				m_collision = CollisionType( Reflex::Circle( transform->GetWorldPosition(), sfmlObj->GetCircleShape().getRadius() ) );
			break;
			case SFMLObjectType::Rectangle:
				m_collision = CollisionType( Reflex::ToAABB( transform->GetWorldTransform().transformRect( sfmlObj->GetRectangleShape().getLocalBounds() ) ) );
			break;
			case SFMLObjectType::Convex:
				m_collision = CollisionType( Reflex::ToAABB( transform->GetWorldTransform().transformRect( sfmlObj->GetConvexShape().getGlobalBounds() ) ) );
			break;
			case SFMLObjectType::Sprite:
				m_collision = CollisionType( Reflex::ToAABB( transform->GetWorldTransform().transformRect( sfmlObj->GetSprite().getGlobalBounds() ) ) );
			break;
			}
		}

		Interactable::Interactable( const ObjectHandle& object, const BaseHandle& componentHandle, const Reflex::Circle& bounds )
			: Component( object, componentHandle )
			, m_collision( bounds )
		{

		}

		Interactable::Interactable( const ObjectHandle& object, const BaseHandle& componentHandle, const Reflex::AABB& bounds )
			: Component( object, componentHandle )
			, m_collision( bounds )
		{

		}
	}
}