#pragma once

#include "..\ReflexEngine\System.h"

using namespace Reflex::Core;

class GraphRenderer : public Reflex::Systems::System
{
public:
	using System::System;

	void RebuildVertexArray();
	void RegisterComponents() final;

protected:
	void Update( const sf::Time deltaTime ) final;
	void Render( sf::RenderTarget& target, sf::RenderStates states ) const final;
	void OnSystemStartup() final { }
	void OnSystemShutdown() final { }

public:
	sf::VertexArray m_connections;
};