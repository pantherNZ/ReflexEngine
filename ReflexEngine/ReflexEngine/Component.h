#pragma once

#include "Common.h"
#include "Entity.h"

namespace Reflex
{
	namespace Core { class Object; }

	namespace Components
	{
		using Core::Object;
		using Core::BaseHandle;
		using Core::Entity;
		
		class Component : public Entity
		{
		public:
			// Constructors / Destructors
			Component( Object& object, BaseHandle handle );
			virtual ~Component() { }

			Object& GetObject();
			const Object& GetObject() const;

		private:
			Component() = delete;

		protected:
			Object& m_object;
		};
	}
}