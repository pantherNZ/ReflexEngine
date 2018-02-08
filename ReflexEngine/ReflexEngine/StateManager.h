#pragma once

// Includes
#include "Common.h"

#include <functional>

namespace Reflex
{
	namespace Core
	{
		struct Context
		{
			Context( sf::RenderWindow& _window, TextureManager& _textureManager, FontManager& _fontManager )
				: window( &_window )
				, textureManager( &_textureManager )
				, fontManager( &_fontManager )
			{
			}

			sf::RenderWindow* window;
			TextureManager* textureManager;
			FontManager* fontManager;
		};

		class State;

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

			template <typename T>
			void RegisterState( unsigned stateID )
			{
				m_stateFactories[stateID] = [this]()
				{
					return std::make_unique< T >( *this, m_context );
				};
			}

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

			std::vector<PendingChange> m_pendingList;
			std::vector< std::unique_ptr< State > > m_ActiveStates;
			Context m_context;

			std::map< unsigned, std::function< std::unique_ptr< State >( void ) > > m_stateFactories;
		};


		class State
		{
		public:
			State( StateManager& stateManager, Context context );
			virtual ~State() { }

			virtual void Render() = 0;
			virtual bool Update( const sf::Time deltaTime ) = 0;
			virtual bool ProcessEvent( const sf::Event& event ) = 0;

		protected:
			void RequestStackPush( unsigned stateID );
			void RequestStackPop();
			void RequestStateClear();
			const Context& GetContext() const;

		private:
			StateManager& m_stateManager;
			Context m_context;
		};
	}
};