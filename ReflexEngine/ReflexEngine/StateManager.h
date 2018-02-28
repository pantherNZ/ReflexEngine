#pragma once

// Includes
#include "Common.h"
#include "State.h"
#include "Context.h"

namespace Reflex
{
	namespace Core
	{
		class StateManager : private sf::NonCopyable
		{
		public:
			enum Action
			{
				Push,
				Pop,
				Clear,
			};

			explicit StateManager( Context& context );

			template< typename T >
			void RegisterState( unsigned stateID );

			void Update( const sf::Time deltaTime );
			void Render();
			void ProcessEvent( const sf::Event& event );

			void PushState( unsigned stateID );
			void PopState();
			void ClearStates();

			bool IsEmpty() const;

		private:
			std::unique_ptr< State > CreateState( unsigned stateID );
			void ApplyPendingChanges();

		private:
			struct PendingChange
			{
				explicit PendingChange( Action _action, unsigned _stateID = 0 )
					: action( _action )
					, stateID( _stateID )
				{
				}

				Action action;
				unsigned stateID;
			};

			std::vector< PendingChange > m_pendingList;
			std::vector< std::unique_ptr< State > > m_ActiveStates;
			Context m_context;

			std::map< unsigned, std::function< std::unique_ptr< State >( void ) > > m_stateFactories;
		};

		// Template functions
		template< typename T >
		void StateManager::RegisterState( unsigned stateID )
		{
			m_stateFactories[stateID] = [this]()
			{
				return std::make_unique< T >( *this, m_context );
			};
		}
	}
};