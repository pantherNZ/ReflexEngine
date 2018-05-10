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
	}

	namespace Components
	{
		class Component : public Entity
		{
		public:
			// Constructors / Destructors
			Component( const ObjectHandle& object, const BaseHandle& componentHandle );
			virtual ~Component() { }

			ObjectHandle GetObject() const;

		private:
			Component() = delete;

		protected:
			ObjectHandle m_object;
		};
	}
}