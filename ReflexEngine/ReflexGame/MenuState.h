#pragma once

#include "..\ReflexEngine\State.h"

class MenuState : public Reflex::Core::State
{
public:
	MenuState( Reflex::Core::StateManager& stateManager, Reflex::Core::Context context );

protected:
	void Render() final;
	bool Update( const sf::Time deltaTime ) final;
	bool ProcessEvent( const sf::Event& event ) final;

private:
	sf::Sprite m_backgroundSprite;
	sf::Text m_text;
	bool m_showText;
	sf::Time m_textEffectTime;
};