#pragma once

#include "Common.h"

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
			World& mWorld;
		};

		template< class T >
		void System::RequiresComponent()
		{
			mWorld.ForwardRegisterComponent< T >();
		}
	}
}
