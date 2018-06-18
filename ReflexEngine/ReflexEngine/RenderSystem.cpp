#include "World.h"
#include "RenderSystem.h"
#include "SFMLObjectComponent.h"
#include "TransformComponent.h"
#include "HandleFwd.hpp"

#include <algorithm>

namespace Reflex
{
	namespace Systems
	{
		void RenderSystem::RegisterComponents()
		{
			RequiresComponent( Reflex::Components::SFMLObject );
			RequiresComponent( Reflex::Components::Transform );
		}

		void RenderSystem::Update( const float deltaTime )
		{
			std::sort( m_components.begin(), m_components.end(), []( const ComponentsSet& left, const ComponentsSet& right )
			{
				return Handle< Reflex::Components::Transform >( left[1] )->GetRenderIndex() < Handle< Reflex::Components::Transform >( right[1] )->GetRenderIndex();
			} );
		}

		void RenderSystem::Render( sf::RenderTarget& target, sf::RenderStates states ) const
		{
			PROFILE;
			sf::RenderStates copied_states( states );

			ForEachSystemComponent< Reflex::Components::SFMLObject, Reflex::Components::Transform >( 
				[&target, &copied_states, &states]( SFMLObjectHandle object, TransformHandle transform )
			{
				copied_states.transform = states.transform * transform->GetWorldTransform();

				switch( object->GetType() )
				{
				case Components::Rectangle:
					target.draw( object->GetRectangleShape(), copied_states );
				break;
				case Components::Convex:
					target.draw( object->GetConvexShape(), copied_states );
				break;
				case Components::Circle:
					target.draw( object->GetCircleShape(), copied_states );
				break;
				case Components::Sprite:
					target.draw( object->GetSprite(), copied_states );
				break;
				}
			} );
		}

		std::vector< System::ComponentsSet >::const_iterator RenderSystem::GetInsertionIndex( const ComponentsSet& newSet ) const
		{
			return std::lower_bound( m_components.begin(), m_components.end(), newSet, []( const ComponentsSet& left, const ComponentsSet& right )
			{
				return Handle< Reflex::Components::Transform >( left[1] )->GetRenderIndex() < Handle< Reflex::Components::Transform >( right[1] )->GetRenderIndex();
			} );
		}
	}
}