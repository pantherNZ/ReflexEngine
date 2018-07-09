#pragma once

#include "Component.h"
#include "Utility.h"
#include "InteractableSystem.h"
#include "SFMLObjectComponent.h"

namespace Reflex
{
	namespace Components
	{
		class Interactable;
	}

	namespace Core
	{
		typedef Handle< class Reflex::Components::Interactable > InteractableHandle;
	}

	namespace Components
	{
		// Class definition
		class Interactable : public Component
		{
		public:
			friend class Reflex::Systems::InteractableSystem;

			Interactable( const SFMLObjectHandle& collisionObject = SFMLObjectHandle::null );

			// Settings, change as you want
			bool selectionIsToggle = true;
			bool unselectIfLostFocus = false;
			bool isEnabled = true;

			// Callbacks
			std::function< void( const InteractableHandle&, const bool ) > focusChangedCallback;
			std::function< void( const InteractableHandle&, const bool ) > selectionChangedCallback;

			bool IsFocussed() const;
			bool IsSelected() const;

		protected:
			void Select();
			void Deselect();

			bool isFocussed = false;
			bool isSelected = false;

			SFMLObjectHandle m_replaceCollisionObject;
		};
	}
}