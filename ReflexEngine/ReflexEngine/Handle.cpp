#include "Handle.h"
#include "HandleManager.h"

namespace Reflex
{
	namespace Core
	{
		template< class T >
		HandleManager* Handle< t >::handleManager = nullptr;

		template< class T >
		Handle::Handle()
			: m_index( -1 )
			, m_counter( -1 )
		{
		}

		template< class T >
		Handle::Handle( uint32_t index, uint32_t counter )
			: m_index( index )
			, m_counter( counter )
		{
		}

		template< class T >
		Handle::Handle( uint32_t handle )
		{
			// Cast this to an unsigned pointer, deference and then set this to the handle (which will set each member respectively)
			*( ( uint32_t* )this ) = handle;
		}

		template< class T >
		inline T* Handle::Get() const
		{
			return handleManager->GetAs< T >( *this );
		}

		template< class T >
		inline T* Handle::operator ->()
		{
			return Get< T >();
		}

		template< class T >
		inline Handle::operator unsigned() const
		{
			return m_counter << 16 | m_index;
		}

		template< class T >
		inline bool Handle::IsValid() const
		{
			return m_index != -1 && m_counter != -1;
		}
	}
}