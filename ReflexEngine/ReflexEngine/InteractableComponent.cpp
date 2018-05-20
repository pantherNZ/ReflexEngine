#include "InteractableComponent.h"
#include "SFMLObjectComponent.h"
#include "Object.h"

namespace Reflex
{
	namespace Components
	{
		void Interactable::Select()
		{
			if( !m_isSelected )
			{
				m_isSelected = true;

				if( m_selectedCallback )
					m_selectedCallback( Handle< Interactable >( m_self ) );
			}
		}

		void Interactable::Deselect()
		{
			if( m_isSelected )
			{
				m_isSelected = false;

				if( m_deselectedCallback )
					m_deselectedCallback( Handle< Interactable >( m_self ) );
			}
		}
	}
}