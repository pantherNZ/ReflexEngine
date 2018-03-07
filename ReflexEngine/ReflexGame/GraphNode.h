#pragma once

#include "..\ReflexEngine\Component.h"

class GraphNode : public Reflex::Components::Component
{
public:
	using Component::Component;
	std::vector< GraphNode* > m_connections;
};