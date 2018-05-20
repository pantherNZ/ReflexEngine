#pragma once

#include "Handle.h"
#include <SFML\System\NonCopyable.hpp>

namespace Reflex
{
	namespace Core
	{
		class Entity
		{
		public:
			virtual ~Entity() { }
			BaseHandle m_self;
		};
	}
}