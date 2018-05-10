#include "World.h"
#include "InteractableSystem.h"
#include "InteractableComponent.h"

namespace Reflex
{
	namespace Systems
	{
		void InteractableSystem::RegisterComponents()
		{
			RequiresComponent( Reflex::Components::Transform );
			RequiresComponent( Reflex::Components::Interactable );
		}
	}
}