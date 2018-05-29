#pragma once

#include <stdint.h>

namespace Reflex
{
	namespace Core
	{
		class HandleManager;

		class BaseHandle
		{
		public:
			BaseHandle();
			BaseHandle( uint32_t index, uint32_t counter );
			BaseHandle( uint32_t handle );

			explicit operator bool() const;
			operator unsigned() const;
			bool operator==( const BaseHandle& other ) const;
			bool operator!=( const BaseHandle& other ) const;
			virtual bool IsValid() const;

			// Members
			uint32_t m_index : 16;
			uint32_t m_counter : 16;
			bool markedForDeletion = false;

			static BaseHandle null;
			static class HandleManager* s_handleManager;
		};

		template< class T >
		class Handle : public BaseHandle
		{
		public:
			Handle() : BaseHandle() {}
			Handle( const BaseHandle& handle );
			T* Get() const;
			T* operator->() const;
			bool IsValid() const final;

			template< class V >
			Handle( const Handle< V >& handle ) = delete;
		};
	}
}

#include "HandleFwd.hpp"