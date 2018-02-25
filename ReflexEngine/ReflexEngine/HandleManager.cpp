#include "HandleManager.h"

namespace Reflex
{
	namespace Core
	{
		HandleManager::HandleManager()
			: m_freeSlots( m_MaxEntries )
			, m_freeList( 0 )
		{
			Clear();
		}

		HandleManager::HandleEntry::HandleEntry()
			: m_nextFreeIndex( 0 )
			, m_counter( 0 )
			, m_endOfList( false )
			, m_allocated( false )
			, m_ptr( NULL )
		{
		}

		HandleManager::HandleEntry::HandleEntry( unsigned nextFreeIndex )
			: m_nextFreeIndex( nextFreeIndex )
			, m_counter( 0 )
			, m_endOfList( false )
			, m_allocated( false )
			, m_ptr( NULL )
		{
		}

		void HandleManager::Clear( void )
		{
			// Link free slots together
			for( unsigned i = 0; i < m_MaxEntries - 1; ++i )
				m_array[i] = HandleEntry( i + 1 );
			m_array[m_MaxEntries - 1].m_endOfList = true;

			m_freeSlots = m_MaxEntries;
		}

		unsigned HandleManager::FreeSlots() const
		{
			return m_freeSlots;
		}
	}
}