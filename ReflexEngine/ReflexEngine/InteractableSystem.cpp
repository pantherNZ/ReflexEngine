#include "World.h"
#include "InteractableSystem.h"
#include "InteractableComponent.h"

using namespace Reflex::Components;

namespace Reflex
{
	namespace Systems
	{
		void InteractableSystem::RegisterComponents()
		{
			RequiresComponent( Transform );
			RequiresComponent( Interactable );
		}

		void InteractableSystem::Update( const float deltaTime )
		{
			const auto mouse_position = sf::Mouse::getPosition( *GetWorld().GetContext().window );

			ForEachSystemComponent< Transform, Interactable >(
				[&]( const TransformHandle& transform, const InteractableHandle& interactable )
			{


			} );
		}
	}
}