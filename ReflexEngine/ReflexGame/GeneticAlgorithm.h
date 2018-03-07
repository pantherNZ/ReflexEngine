#pragma once

#include "..\ReflexEngine\System.h"

class GeneticAlgorithm : public Reflex::Systems::System
{
public:
	GeneticAlgorithm( Reflex::Core::World& m_world ) : System( m_world ) { }

protected:
	void RegisterComponents() final;
	void Update( const sf::Time deltaTime ) final;
	void OnSystemStartup() final { }
	void OnSystemShutdown() final { }
};