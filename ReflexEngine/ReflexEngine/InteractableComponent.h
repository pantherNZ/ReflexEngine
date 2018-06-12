#pragma once

#include "Component.h"
#include "Utility.h"
#include "InteractableSystem.h"

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

			// Settings, change as you want
			bool selectionIsToggle = true;
			bool unselectIfLostFocus = false;
			bool isEnabled = true;

			// Callbacks
			std::function< void( const InteractableHandle& ) > gainedFocusCallback;
			std::function< void( const InteractableHandle& ) > lostFocusCallback;
			std::function< void( const InteractableHandle& ) > selectedCallback;
			std::function< void( const InteractableHandle& ) > deselectedCallback;

			bool IsFocussed() const;
			bool IsSelected() const;

		protected:
			bool isFocussed = false;
			bool isSelected = false;

			void Select();
			void Deselect();
		};
	}
}