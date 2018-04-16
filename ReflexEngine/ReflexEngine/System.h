#pragma once

#include "Common.h"
#include "Handle.h"

namespace Reflex
{
	namespace Core { class World; }

	namespace Systems
	{
		using namespace Reflex::Core;

#define RequiresComponent( T ) GetWorld().ForwardRegisterComponent< T >(); \
		m_requiredComponentTypes.push_back( Type( typeid( T ) ) );

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
			World& GetWorld() { return m_world; }

		protected:
			template< typename T >
			Handle< T > GetSystemComponent( const std::vector< Reflex::Core::BaseHandle >& set ) const
			{
				const auto type = Type( typeid( T ) );
				for( unsigned i = 0U; i < m_requiredComponentTypes.size(); ++i )
					if( m_requiredComponentTypes[i] == type )
						return Handle< T >( set[i] );
				return Handle< T >();
			}

		private:
			void draw( sf::RenderTarget& target, sf::RenderStates states ) const final { Render( target, states ); }

		protected:
			// Iterating this causes a lot of jumping around in memory, it isn't very efficient (if there is more then one required component)
			std::vector< std::vector< Reflex::Core::BaseHandle > > m_components;
			std::vector< Type > m_requiredComponentTypes;

		private:
			World& m_world;
		};
	}
}
