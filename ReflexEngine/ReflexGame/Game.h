#pragma once

#include "..\ReflexEngine\Engine.h"

class Game : public Reflex::Core::Engine
{
public:
	Game();

	enum Scenes : unsigned short
	{
		MenuScene,
		GraphScene,
	};

protected:
	unsigned GetStartupState() const override;
	void RegisterStates() override;
	void OnPostSetup() override;

private:
};