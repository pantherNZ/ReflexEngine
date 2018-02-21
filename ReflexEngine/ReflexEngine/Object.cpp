#include "Object.h"
#include "World.h"

namespace Reflex
{
	namespace Core
	{
		Object::Object( World& world )
			: mWorld( world )
		{

		}

		void Object::Destroy()
		{
			mWorld.DestroyObject( *this );
		}

		void Object::draw( sf::RenderTarget& target, sf::RenderStates states ) const
		{
			states.transform *= getTransform();

			Draw( target, states );
		}
	}
}