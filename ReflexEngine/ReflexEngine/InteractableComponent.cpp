#include "InteractableComponent.h"
#include "SFMLObjectComponent.h"
#include "Object.h"

namespace Reflex
{
	namespace Components
	{
		Interactable::Interactable( const SFMLObjectHandle& collisionObject )
			: Component()
			, m_replaceCollisionObject( collisionObject )
		{

		}

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

				if( selectionChangedCallback )
					selectionChangedCallback( Handle< Interactable >( m_self ), true );
			}
		}

		void Interactable::Deselect()
		{
			if( isSelected )
			{
				isSelected = false;

				if( selectionChangedCallback )
					selectionChangedCallback( Handle< Interactable >( m_self ), false );
			}
		}
	}
}