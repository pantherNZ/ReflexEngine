#pragma once

#include "Common.h"
#include "Handle.h"

namespace Reflex
{
	namespace Core { class World; }

	namespace Systems
	{
		using namespace Reflex::Core;

#define RequiresComponent( T ) m_world.ForwardRegisterComponent< T >(); m_requiredComponentTypes.push_back( Type( typeid( T ) ) );

		class System : private sf::NonCopyable, public sf::Drawable
		{
			friend class World;

		public:
			//Constructors / Destructors
			System( World& world ) : m_world( world ) { }
			virtual ~System() { }

			virtual void RegisterComponents() = 0;
			virtual void Update( const sf::Time deltaTime ) { }
			virtual void Render( sf::RenderTarget& target, sf::RenderStates states ) const { }
			virtual void OnSystemStartup() { }
			virtual void OnSystemShutdown() { }

			const std::vector< Type >& GetRequiredComponentTypes() const { return m_requiredComponentTypes; }

		protected:
			//template< typename Func, typename... Args >
			//void ForEachComponent( Func f )
			//{
			//	for( auto& component : m_components )
			//	{
			//		f( 
			//
			//		for( unsigned index = 0U; index < component.size(); ++index )
			//			CastComponent< Func, Args... >( f, component[index] );
			//	}
			//}

		private:
			void draw( sf::RenderTarget& target, sf::RenderStates states ) const final { Render( target, states ); }

			//template< typename Func, typename T, typename... Args >
			//Args... CastComponent( Func f, BaseHandle& handle )
			//{
			//	return Handle< T >( handle );
			//}

		protected:
			World& m_world;
			// Iterating this causes a lot of jumping around in memory, it isn't very efficient (if there is more then one required component)
			std::vector< std::vector< Reflex::Core::BaseHandle > > m_components;
			std::vector< Type > m_requiredComponentTypes;
		};
	}
}
