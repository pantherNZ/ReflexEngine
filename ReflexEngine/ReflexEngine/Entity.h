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
			explicit Entity( const BaseHandle& handle ) : m_self( handle ) { }
			BaseHandle m_self;
		};
	}
}