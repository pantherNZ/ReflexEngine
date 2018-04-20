#pragma once

#include "Precompiled.h"
#include "Entity.h"

namespace Reflex
{
	using namespace Reflex::Core;

	namespace Components
	{
		class Component : public Entity
		{
		public:
			// Constructors / Destructors
			Component( ObjectHandle object, BaseHandle handle );
			virtual ~Component() { }

			ObjectHandle GetObject() const;

		private:
			Component() = delete;

		protected:
			ObjectHandle m_object;
		};
	}
}