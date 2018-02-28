#include "HandleManager.h"

namespace Reflex
{
	namespace Core
	{
		template< class T >
		HandleManager* Handle< T >::s_handleManager = nullptr;

		template< class T >
		Handle< T >::Handle()
			: m_index( -1 )
			, m_counter( -1 )
		{
		}

		template< class T >
		Handle< T >::Handle( uint32_t index, uint32_t counter )
			: m_index( index )
			, m_counter( counter )
		{
		}

		template< class T >
		Handle< T >::Handle( uint32_t handle )
		{
			// Cast this to an unsigned pointer, deference and then set this to the handle (which will set each member respectively)
			*( ( uint32_t* )this ) = handle;
		}

		template< class T >
		T* Handle< T >::Get() const
		{
			return s_handleManager->GetAs< T >( *this );
		}

		template< class T >
		T* Handle< T >::operator ->() const
		{
			return Get< T >();
		}

		template< class T >
		Handle< T >::operator bool() const
		{
			return IsValid();
		}

		template< class T >
		Handle< T >::operator unsigned() const
		{
			return m_counter << 16 | m_index;
		}

		template< class T >
		bool Handle< T >::IsValid() const
		{
			return m_index != -1 && m_counter != -1;
		}
	}
}