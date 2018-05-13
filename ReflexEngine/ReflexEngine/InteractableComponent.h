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
			Interactable( const ObjectHandle& object, const BaseHandle& componentHandle );
			Interactable( const ObjectHandle& object, const BaseHandle& componentHandle, const Reflex::AABB& bounds );
			Interactable( const ObjectHandle& object, const BaseHandle& componentHandle, const Reflex::Circle& bounds );

			bool m_isFocussed = false;
			bool m_isSelected = false;

			std::function< void( const InteractableHandle& ) > m_gainedFocusCallback;
			std::function< void( const InteractableHandle& ) > m_lostFocusCallback;
			std::function< void( const InteractableHandle& ) > m_selectedCallback;
			std::function< void( const InteractableHandle& ) > m_unselectedCallback;

			union CollisionType
			{
				Reflex::AABB boxBounds;
				Reflex::Circle circleBounds;

				CollisionType() { }
				CollisionType( const Reflex::AABB& boxBounds ) : boxBounds( boxBounds ) { }
				CollisionType( const Reflex::Circle& circleBounds ) : circleBounds( circleBounds ) { }
				~CollisionType() { }
			};

			CollisionType m_collision;
		};
	}
}