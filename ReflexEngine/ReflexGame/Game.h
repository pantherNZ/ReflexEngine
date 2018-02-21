#pragma once

#include "..\ReflexEngine\Engine.h"

class Game : public Reflex::Core::Engine
{
public:
	Game();

	enum States : unsigned short
	{
		MenuState,
		GraphState,
	};

protected:
	unsigned GetStartupState() const override;
	void RegisterStates() override;
	void OnPostSetup() override;

private:
};