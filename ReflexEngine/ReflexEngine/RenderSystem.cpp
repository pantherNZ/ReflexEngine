#include "World.h"
#include "RenderSystem.h"
#include "SpriteComponent.h"
#include "TransformComponent.h"

namespace Reflex
{
	namespace Systems
	{
		void RenderSystem::RegisterComponents()
		{
			RequiresComponent< Reflex::Components::SpriteComponent >();
			RequiresComponent< Reflex::Components::TransformComponent >();
		}

		void RenderSystem::Render( sf::RenderTarget& target, sf::RenderStates states ) const
		{
			sf::RenderStates copied_states( states );

			for( auto& component : m_components )
			{
				auto* transform = Reflex::Core::Handle< Reflex::Components::TransformComponent >( component.back() ).Get();
				copied_states.transform = states.transform * transform->getTransform();

				auto* sprite = Reflex::Core::Handle< Reflex::Components::SpriteComponent >( component.front() ).Get();
				target.draw( *sprite, copied_states );
			}
		}
	}
}