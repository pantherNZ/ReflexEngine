#pragma once

#include "ComponentTypes.h"

namespace Reflex
{
	namespace Core { class Object; }

	namespace Components
	{
		class Component
		{
		public:
			//Constructors / Destructors
			Component( Object& object ) : mObject( object ) { }
			virtual ~Component() { }

			//Called when the object is fully constructed (all other components are available)
			virtual void OnConstructionComplete() { };

			Object &GetObject() { return mObject; }
			const Object& GetObject() const { return mObject; }

		private:
			Object& mObject;

			//Remove copy and assign
			Component( const Component& ) = delete;
			Component& operator=( const Component& ) = delete;
		};
	}
}