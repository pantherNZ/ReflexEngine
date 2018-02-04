#pragma once

#include "..\ReflexEngine\Object.h"
#include "..\ReflexEngine\Engine.h"

class GraphNode : public Reflex::Core::Object
{
public:
	GraphNode();// const Reflex::Core::TextureManager& textureManager );

protected:
	void DrawCurrent( sf::RenderTarget& target, sf::RenderStates states ) const final;

private:
	sf::Sprite m_sprite;
};