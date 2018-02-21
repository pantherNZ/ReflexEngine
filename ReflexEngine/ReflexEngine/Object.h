#pragma once

#include "Common.h"
#include "Handle.h"
#include "ComponentTypes.h"

namespace Reflex
{
	namespace Core
	{
		class World;

		class Object : public sf::Transformable, public sf::Drawable, private sf::NonCopyable
		{
		public:
			Object( World& world );

			void Destroy();

		protected:
			virtual void Draw( sf::RenderTarget& target, sf::RenderStates states ) const { }

		private:
			Object() = delete;
			void draw( sf::RenderTarget& target, sf::RenderStates states ) const final;

		private:
			World& mWorld;
			unsigned mComponents[Reflex::Components::NumComponents];
		};
	}
}