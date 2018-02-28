#pragma once

#include <cstdint>

namespace Reflex
{
	namespace Core
	{
		class HandleManager;

		template< class T >
		struct Handle
		{
			Handle();
			Handle( uint32_t index, uint32_t counter );
			Handle( uint32_t handle );

			T* Get() const;
			T* operator ->() const;
			explicit operator bool() const;
			operator unsigned() const;
			bool IsValid() const;

			// Members
			uint32_t m_index : 16;
			uint32_t m_counter : 16;

			// Static handle manager which is initialised by the World on startup
			static class HandleManager* s_handleManager;
		};
	}
}