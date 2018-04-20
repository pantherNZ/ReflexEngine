#pragma once

#include "Context.h"

namespace Reflex
{
	namespace Core
	{
		class StateManager;

		class State
		{
		public:
			State( StateManager& stateManager, Context context );
			virtual ~State() { }

			virtual void Render() = 0;
			virtual bool Update( const float deltaTime ) = 0;
			virtual bool ProcessEvent( const sf::Event& event ) = 0;

		protected:
			void RequestStackPush( unsigned stateID );
			void RequestStackPop();
			void RequestStateClear();
			const Context& GetContext() const;

		private:
			State();

		private:
			StateManager& m_stateManager;
			Context m_context;
		};
	}
}