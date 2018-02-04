#pragma once

// Includes
#include "CommonIncludes.h"

namespace Reflex
{
	namespace Core
	{
		struct Context 
		{
			Context( sf::RenderWindow& _window ) // TextureHolder& textures, FontHolder& fonts, Player& player );
				: window( &_window )
			{ }

			sf::RenderWindow* window;
			//TextureHolder* textures;
			//FontHolder* fonts;
		};

		class Scene
		{
		public:
			Scene( SceneManager& stack, Context context );
			virtual ~State();
			virtual void draw() = 0;
			virtual bool update( sf::Time dt ) = 0;
			virtual bool handleEvent( const sf::Event& event ) = 0;
		protected:
			void requestStackPush( States::ID stateID );
			void requestStackPop();
			void requestStateClear();
			Context getContext() const;
		private:
			SceneManager& m_scene_manager;
			Context m_context;
		};

		class SceneManager : private sf::NonCopyable
		{
		public:
			enum Action
			{
				Push,
				Pop,
				Clear,
			};
			explicit StateStack( State::Context context );
			template <typename T>
			void registerState( States::ID stateID );
			void Update( const sf::Time delta_time );
			void Render();
			void ProcessEvent( const sf::Event& event );
			void pushState( States::ID stateID );
			void popState();
			void clearStates();
			bool isEmpty() const;
		private:
			std::unique_ptr<State> createState( States::ID stateID );
			void applyPendingChanges();
		private:
			struct PendingChange
			{
				...
					Action action;
				States::ID stateID;
			};
		private:
			std::vector<State::Ptr> mStack;
			std::vector<PendingChange> mPendingList;
			State::Context mContext;
			std::map<States::ID, std::function<State::Ptr()>> mFactories;
		};
	}
};