#pragma once

#include "Component.h"
#include "Utility.h"

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
			bool m_isFocussed = false;
			bool m_isSelected = false;
			bool m_selectionIsToggle = true;
			bool m_unselectIfLostFocus = false;

			void Select();
			void Deselect();

			std::function< void( const InteractableHandle& ) > m_gainedFocusCallback;
			std::function< void( const InteractableHandle& ) > m_lostFocusCallback;
			std::function< void( const InteractableHandle& ) > m_selectedCallback;
			std::function< void( const InteractableHandle& ) > m_deselectedCallback;
		};
	}
}