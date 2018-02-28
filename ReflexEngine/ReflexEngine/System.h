#pragma once

#include "Common.h"
#include "Component.h"

namespace Reflex
{
	namespace Core { class World; }

	namespace Systems
	{
		using Core::World;

		class System : private sf::NonCopyable
		{
		public:
			//Constructors / Destructors
			System( World& mWorld );
			virtual ~System() { }

			virtual void RegisterComponents() = 0;
			virtual void Update( const sf::Time deltaTime ) { }
			virtual void OnSystemStartup() { }
			virtual void OnSystemShutdown() { }

		protected:
			template< class T >
			void RequiresComponent();

		protected:
			World& m_world;
			// Iterating this causes a lot of jumping around in memory, it isn't very efficient (if there is more then one required component)
			std::vector< std::vector< Reflex::Components::ComponentHandle > > m_components;
		};

		template< class T >
		void System::RequiresComponent()
		{
			m_world.ForwardRegisterComponent< T >();
		}
	}
}
