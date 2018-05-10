#pragma once

#include "Component.h"

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
			Interactable( const ObjectHandle& object, const BaseHandle& componentHandle, std::function< void( const InteractableHandle& ) > highlightedCallback, std::function< void( const InteractableHandle& ) > selectedCallback );

			Interactable( const ObjectHandle& object, const BaseHandle& componentHandle, const AABB& bounds, 
				std::function< void( const InteractableHandle& ) > highlightedCallback, 
				std::function< void( const InteractableHandle& ) > selectedCallback );

			Interactable( const ObjectHandle& object, const BaseHandle& componentHandle, const Circle& bounds, 
				std::function< void( const InteractableHandle& ) > highlightedCallback, 
				std::function< void( const InteractableHandle& ) > selectedCallback );

			bool m_isHighlightable = false;
			bool m_isHighlighted = false;

			bool m_isSelectable = false;
			bool m_isSelected = false;

			std::function< void( const InteractableHandle& ) > m_highlightedCallback;
			std::function< void( const InteractableHandle& ) > m_selectedCallback;

			union CollisionType
			{
				AABB boxBounds;
				Circle circleBounds;

				CollisionType( const AABB& boxBounds ) : boxBounds( boxBounds ) { }
				CollisionType( const Circle& circleBounds ) : circleBounds( circleBounds ) { }
				~CollisionType() { }
			};

			CollisionType m_collision;
		};
	}
}