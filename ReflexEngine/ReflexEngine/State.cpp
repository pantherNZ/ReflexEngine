#include "State.h"
#include "StateManager.h"

namespace Reflex
{
	namespace Core
	{
		State::State( StateManager& stateManager, Context context )
			: m_stateManager( stateManager )
			, m_context( context )
		{

		}

		void State::RequestStackPush( unsigned stateID )
		{
			m_stateManager.PushState( stateID );
		}

		void State::RequestStackPop()
		{
			m_stateManager.PopState();
		}

		void State::RequestStateClear()
		{
			m_stateManager.ClearStates();
		}

		const Context& State::GetContext() const
		{
			return m_context;
		}
	}
}