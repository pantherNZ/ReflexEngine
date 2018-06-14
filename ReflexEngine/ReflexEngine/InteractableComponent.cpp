#include "InteractableComponent.h"
#include "SFMLObjectComponent.h"
#include "Object.h"

namespace Reflex
{
	namespace Components
	{

		bool Interactable::IsFocussed() const
		{
			return isFocussed;
		}

		bool Interactable::IsSelected() const
		{
			return isSelected;
		}

		void Interactable::Select()
		{
			if( !isSelected && isEnabled )
			{
				isSelected = true;

				if( selectedCallback )
					selectedCallback( Handle< Interactable >( m_self ) );
			}
		}

		void Interactable::Deselect()
		{
			if( isSelected )
			{
				isSelected = false;

				if( deselectedCallback )
					deselectedCallback( Handle< Interactable >( m_self ) );
			}
		}
	}
}