#include "StateManager.h"
#include "State.h"

namespace Reflex
{
	namespace Core
	{
		StateManager::StateManager( Context& context )
			: m_context( context )
		{

		}

		void StateManager::Update( const sf::Time deltaTime )
		{
			for( auto itr = m_ActiveStates.rbegin(); itr != m_ActiveStates.rend(); ++itr )
			{
				if( !( *itr )->Update( deltaTime ) )
					break;
			}

			ApplyPendingChanges();
		}

		void StateManager::Render()
		{
			for( auto& state : m_ActiveStates )
				state->Render();
		}

		void StateManager::ProcessEvent( const sf::Event& event )
		{
			for( auto itr = m_ActiveStates.rbegin(); itr != m_ActiveStates.rend(); ++itr )
			{
				if( !( *itr )->ProcessEvent( event ) )
					break;
			}

			ApplyPendingChanges();
		}

		void StateManager::PushState( unsigned stateID )
		{
			m_pendingList.push_back( PendingChange( Push, stateID ) );
		}

		void StateManager::PopState()
		{
			m_pendingList.push_back( PendingChange( Pop ) );
		}

		void StateManager::ClearStates()
		{
			m_pendingList.push_back( PendingChange( Clear ) );
		}

		bool StateManager::IsEmpty() const
		{
			return m_ActiveStates.empty();
		}

		std::unique_ptr< State > StateManager::CreateState( unsigned stateID )
		{
			const auto found = m_stateFactories.find( stateID );

			if( found == m_stateFactories.end() )
				LOG_CRIT( "StateManager::CreateState | Failed to find creation function for state " + stateID );

			return found->second();
		}

		void StateManager::ApplyPendingChanges()
		{
			for( PendingChange change : m_pendingList )
			{
				switch( change.action )
				{
				case Push:
					m_ActiveStates.push_back( CreateState( change.stateID ) );
					break;
				case Pop:
					m_ActiveStates.pop_back();
					break;
				case Clear:
					m_ActiveStates.clear();
					break;
				}
			}

			m_pendingList.clear();
		}
	}
}

