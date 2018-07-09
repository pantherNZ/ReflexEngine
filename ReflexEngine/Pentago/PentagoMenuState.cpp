#include "PentagoMenuState.h"
#include "Resources.h"
#include "..\ReflexEngine\SFMLObjectComponent.h"
#include "..\ReflexEngine\InteractableComponent.h"

PentagoMenuState::PentagoMenuState( StateManager& stateManager, Context context )
	: State( stateManager, context )
	, m_bounds( 0.0f, 0.0f, ( float )context.window->getSize().x, ( float )context.window->getSize().y )
	, m_world( context, m_bounds, 15 )
{
	const auto& font = context.fontManager->LoadResource( Reflex::ResourceID::ArialFont, "Data/Fonts/arial.ttf" );

	const unsigned menuItems = 2;
	const auto boxSize = m_bounds.height * 0.6f;
	sf::Vector2f startPos( m_bounds.width / 4.0f, ( m_bounds.height - boxSize ) / 2.0f );
	const auto offset = sf::Vector2f( 0.0f, m_bounds.height / menuItems );

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
			RequestStackPush( PentagoGameStateType );
		}
	};

	playBtnInteract->focusChangedCallback = [playText]( const InteractableHandle& interactable, const bool focussed )
	{
		playText->GetText().setFillColor( focussed ? sf::Color::Magenta : sf::Color::Red );
	};

	// Exit
	const auto exitBtn = m_world.CreateObject( startPos + offset );
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

	//GetContext().window->draw( m_text[( int )m_playerTurn + ( m_gameOver ? 2 : 0 )] );
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