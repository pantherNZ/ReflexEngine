#include "PentagoMenuState.h"
#include "Resources.h"
#include "..\ReflexEngine\SFMLObjectComponent.h"
#include "..\ReflexEngine\InteractableComponent.h"
#include "PentagoGameState.h"

PentagoMenuState::PentagoMenuState( StateManager& stateManager, Context context )
	: State( stateManager, context )
	, m_bounds( 0.0f, 0.0f, ( float )context.window->getSize().x, ( float )context.window->getSize().y )
	, m_world( context, m_bounds, 15 )
{
	const auto& font = context.fontManager->LoadResource( Reflex::ResourceID::ArialFont, "Data/Fonts/arial.ttf" );
	const auto& helpTexture = context.textureManager->LoadResource( Reflex::ResourceID::HelpScreen, "Data/Textures/HelpScreen.png" );

	const unsigned menuItems = 3;
	const auto boxSize = m_bounds.height * 0.3f;
	sf::Vector2f startPos( m_bounds.width / 4.0f, m_bounds.height / 2.0f - boxSize / 2.0f );
	const auto offset = sf::Vector2f( 0.0f, boxSize / ( menuItems - 1 ) );

	// Play game
	const auto playBtn = m_world.CreateObject( startPos );
	const auto playText = playBtn->AddComponent< Reflex::Components::SFMLObject >( sf::Text( "Play", font ), sf::Color::Red );
	const auto playCollision = playBtn->AddComponent< Reflex::Components::SFMLObject >( sf::RectangleShape( sf::Vector2f( 400.0f, 100.0f ) ) );

	auto playBtnInteract = playBtn->AddComponent< Reflex::Components::Interactable >( playCollision );
	playBtnInteract->selectionIsToggle = false;

	playBtnInteract->selectionChangedCallback = [this]( const InteractableHandle& interactable, const bool selected )
	{
		if( selected )
		{
			RequestStateClear();
			RequestStackPush( SetDifficultyStateType );
		}
	};

	playBtnInteract->focusChangedCallback = [playText]( const InteractableHandle& interactable, const bool focussed )
	{
		playText->GetText().setFillColor( focussed ? sf::Color::Magenta : sf::Color::Red );
	};

	// Help
	const auto helpBtn = m_world.CreateObject( startPos + offset );
	const auto helpText = helpBtn->AddComponent< Reflex::Components::SFMLObject >( sf::Text( "Help", font ), sf::Color::Red );
	const auto helpCollision = helpBtn->AddComponent< Reflex::Components::SFMLObject >( sf::RectangleShape( sf::Vector2f( 400.0f, 100.0f ) ) );

	auto helpBtnInteract = helpBtn->AddComponent< Reflex::Components::Interactable >( helpCollision );
	helpBtnInteract->selectionIsToggle = false;

	helpBtnInteract->selectionChangedCallback = [this, &helpTexture]( const InteractableHandle& interactable, const bool selected )
	{
		if( selected )
		{
			if( !m_helpDisplay )
			{
				m_helpDisplay = m_world.CreateObject( sf::Vector2f( m_bounds.width / 2.0f + m_bounds.width / 8.0f, m_bounds.height / 2.0f ) );
				auto sprite = m_helpDisplay->AddComponent< Reflex::Components::SFMLObject >( sf::Sprite( helpTexture ) );
			}
			else
			{
				m_helpDisplay->Destroy();
			}
		}
	};

	helpBtnInteract->focusChangedCallback = [helpText]( const InteractableHandle& interactable, const bool focussed )
	{
		helpText->GetText().setFillColor( focussed ? sf::Color::Magenta : sf::Color::Red );
	};

	// Exit
	const auto exitBtn = m_world.CreateObject( startPos + offset * 2.0f );
	const auto exitText = exitBtn->AddComponent< Reflex::Components::SFMLObject >( sf::Text( "Exit", font ), sf::Color::Red );
	const auto exitCollision = exitBtn->AddComponent< Reflex::Components::SFMLObject >( sf::RectangleShape( sf::Vector2f( 400.0f, 100.0f ) ) );

	auto exitBtnInteract = exitBtn->AddComponent< Reflex::Components::Interactable >( exitCollision );
	exitBtnInteract->selectionIsToggle = false;

	exitBtnInteract->selectionChangedCallback = [this]( const InteractableHandle& interactable, const bool selected )
	{
		if( selected )
			GetContext().window->close();
	};

	exitBtnInteract->focusChangedCallback = [exitText]( const InteractableHandle& interactable, const bool focussed )
	{
		exitText->GetText().setFillColor( focussed ? sf::Color::Magenta : sf::Color::Red );
	};
}

void PentagoMenuState::Render()
{
	m_world.Render();
}

bool PentagoMenuState::Update( const float deltaTime )
{
	m_world.Update( deltaTime );

	return true;
}

bool PentagoMenuState::ProcessEvent( const sf::Event& event )
{
	m_world.ProcessEvent( event );

	return true;
}

SetDifficultyState::SetDifficultyState( StateManager& stateManager, Context context )
	: State( stateManager, context )
	, m_bounds( 0.0f, 0.0f, ( float )context.window->getSize().x, ( float )context.window->getSize().y )
	, m_world( context, m_bounds, 15 )
{
	const auto& font = context.fontManager->GetResource( Reflex::ResourceID::ArialFont );

	const unsigned menuItems = 3;
	const auto boxSize = m_bounds.width * 0.4f;
	sf::Vector2f startPos( m_bounds.width / 2.0f - boxSize / 2.0f, m_bounds.height / 2.0f );
	const auto offset = sf::Vector2f( boxSize / ( menuItems - 1 ), 0.0f );

	// Play game
	const std::string buttonText[] = { "Easy", "Medium", "Hard" };

	for( unsigned i = 0U; i < 3; ++i )
	{
		const auto btn = m_world.CreateObject( startPos + offset * ( float )i );
		const auto text = btn->AddComponent< Reflex::Components::SFMLObject >( sf::Text( buttonText[i], font ), sf::Color::Red );
		const auto collision = btn->AddComponent< Reflex::Components::SFMLObject >( sf::RectangleShape( sf::Vector2f( 400.0f, 100.0f ) ) );

		auto btnInteract = btn->AddComponent< Reflex::Components::Interactable >( collision );
		btnInteract->selectionIsToggle = false;

		btnInteract->selectionChangedCallback = [this, i]( const InteractableHandle& interactable, const bool selected )
		{
			if( selected )
			{
				PentagoGameState::m_AIDifficulty = i + 1U;
				RequestStateClear();
				RequestStackPush( PentagoGameStateType );
			}
		};

		btnInteract->focusChangedCallback = [text]( const InteractableHandle& interactable, const bool focussed )
		{
			text->GetText().setFillColor( focussed ? sf::Color::Magenta : sf::Color::Red );
		};
	}
}

void SetDifficultyState::Render()
{
	m_world.Render();
}

bool SetDifficultyState::Update( const float deltaTime )
{
	m_world.Update( deltaTime );

	return true;
}

bool SetDifficultyState::ProcessEvent( const sf::Event& event )
{
	m_world.ProcessEvent( event );

	return true;
}