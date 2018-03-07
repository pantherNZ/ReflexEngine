#pragma once

#include "Handle.h"
#include <SFML\System\NonCopyable.hpp>

namespace Reflex
{
	namespace Core
	{
		class Entity : private sf::NonCopyable
		{
		public:
			explicit Entity( const BaseHandle& handle ) : m_self( handle ), m_active( true ) { }
			BaseHandle m_self;
			bool m_active = false;
		};
	}
}