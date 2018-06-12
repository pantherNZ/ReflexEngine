#pragma once

#include "Precompiled.h"
#include "Entity.h"

namespace Reflex
{
	using namespace Reflex::Core;

	namespace Components { class Component; }

	namespace Core
	{
		typedef Handle< class Reflex::Components::Component > ComponentHandle;
		class World;
	}

	namespace Components
	{
		class Component : public Entity
		{
		public:
			friend Reflex::Core::World;

			virtual void OnConstructionComplete() { }

			ObjectHandle GetObject() const { return m_object; }

			virtual void SetOwningObject( const ObjectHandle& owner ) { m_object = owner; }

		protected:
			Component() { }
			virtual ~Component() { }

		protected:
			ObjectHandle m_object;
		};
	}
}