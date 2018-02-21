#pragma once

#include "..\ReflexEngine\Object.h"
#include "..\ReflexEngine\Engine.h"

class GraphNode
{
public:
	GraphNode( const sf::Font& font, const std::string _text );

	sf::CircleShape m_image;
	sf::Text m_label;
	std::vector< GraphNode* > m_connections;

protected:
	//void DrawCurrent( sf::RenderTarget& target, sf::RenderStates states ) const final;
	//void UpdateCurrent( const sf::Time deltaTime ) final { }
};