#include "InteractableComponent.h"

namespace Reflex
{
	namespace Components
	{
		Interactable::Interactable( const ObjectHandle& object, const BaseHandle& componentHandle, std::function< void( const InteractableHandle& ) > highlightedCallback, std::function< void( const InteractableHandle& ) > selectedCallback )
			: Component( object, componentHandle )
			, m_isHighlightable( true )
			, m_isSelectable( true )
			, m_highlightedCallback( highlightedCallback )
			, m_selectedCallback( selectedCallback )
		{

		}

		Interactable::Interactable( const ObjectHandle& object, const BaseHandle& componentHandle, const AABB& bounds, 
			std::function< void( const InteractableHandle& ) > highlightedCallback, 
			std::function< void( const InteractableHandle& ) > selectedCallback )
			: Component( object, componentHandle )
			, m_isHighlightable( true )
			, m_isSelectable( true )
			, m_highlightedCallback( highlightedCallback )
			, m_selectedCallback( selectedCallback )
		{

		}

		Interactable::Interactable( const ObjectHandle& object, const BaseHandle& componentHandle, const Circle& bounds, 
			std::function< void( const InteractableHandle& ) > highlightedCallback, 
			std::function< void( const InteractableHandle& ) > selectedCallback )
			: Component( object, componentHandle )
			, m_isHighlightable( true )
			, m_isSelectable( true )
			, m_highlightedCallback( highlightedCallback )
			, m_selectedCallback( selectedCallback )
		{


		}
	}
}