#pragma once

#include "..\ReflexEngine\Component.h"

class GraphNode : Reflex::Components::Component
{
public:
	GraphNode( Reflex::Core::Object& object, const sf::Font& font, const sf::String text );

	sf::CircleShape m_image;
	sf::Text m_label;
	std::vector< GraphNode* > m_connections;
};