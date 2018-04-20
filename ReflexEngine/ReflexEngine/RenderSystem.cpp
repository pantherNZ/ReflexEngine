#include "World.h"
#include "RenderSystem.h"
#include "SFMLObjectComponent.h"
#include "TransformComponent.h"

namespace Reflex
{
	namespace Systems
	{
		void RenderSystem::RegisterComponents()
		{
			RequiresComponent( Reflex::Components::SFMLObject );
			RequiresComponent( Reflex::Components::Transform );
		}

		void RenderSystem::Render( sf::RenderTarget& target, sf::RenderStates states ) const
		{
			PROFILE;
			sf::RenderStates copied_states( states );

			for( auto& component : m_components )
			{
				auto* transform = Reflex::Core::Handle< Reflex::Components::Transform >( component.back() ).Get();
				copied_states.transform = states.transform * transform->getTransform();

				auto* object = Reflex::Core::Handle< Reflex::Components::SFMLObject >( component.front() ).Get();

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
			}
		}
	}
}