#pragma once

#include "..\ReflexEngine\System.h"

using namespace Reflex::Core;

class GeneticAlgorithm : public Reflex::Systems::System
{
public:
	using System::System;

protected:
	void RegisterComponents() final;
	void Update( const sf::Time deltaTime ) final;
	void Render( sf::RenderTarget& target, sf::RenderStates states ) const final;
	void OnSystemStartup() final;
	void OnSystemShutdown() final { }

private:
	sf::VertexArray m_connections;
};