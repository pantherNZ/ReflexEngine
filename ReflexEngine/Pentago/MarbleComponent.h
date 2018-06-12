#pragma once

#include "..\ReflexEngine\Component.h"

// Class definition
struct Marble : public Reflex::Components::Component
{
	explicit Marble( bool isPlayer ) : isPlayer( isPlayer ) { }
	bool isPlayer = true;
};