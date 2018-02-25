#pragma once

#include "Handle.h"
#include "Object.h"

namespace Reflex
{
	namespace Components
	{
		using Core::Object;
		using Core::Handle;

		typedef Handle< Component > ComponentHandle;

		class Component : private sf::NonCopyable
		{
		public:
			// Constructors / Destructors
			Component( Object& object );
			virtual ~Component() { }

			Object &GetObject();
			const Object& GetObject() const;

		protected:
			Object& mObject;
			ComponentHandle self;
		};
	}
}