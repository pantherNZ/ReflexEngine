#pragma once

#include "Common.h"
#include "Handle.h"

namespace Reflex
{
	namespace Core { class Object; }

	namespace Components
	{
		using Core::Object;
		using Core::Handle;

		typedef Handle< class Component > ComponentHandle;

		class Component : private sf::NonCopyable
		{
		public:
			// Constructors / Destructors
			Component( Object& object );
			virtual ~Component() { }

			Object &GetObject();
			const Object& GetObject() const;

		private:
			Component();

		protected:
			Object& mObject;
			ComponentHandle self;
		};
	}
}