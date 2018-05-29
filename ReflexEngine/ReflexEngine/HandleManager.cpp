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

		BaseHandle HandleManager::Insert( void* ptr )
		{
			HandleEntry* entry = m_array.data() + m_freeList;

			// No more free entries
			assert( !entry->m_endOfList );

			// Cannot insert into an allocated location
			assert( !entry->m_allocated );

			entry->m_allocated = true;

			// Increment freeList
			unsigned index = m_freeList;
			m_freeList = m_array[m_freeList].m_nextFreeIndex;

			// Insert ptr into entry
			entry->m_ptr = ptr;

			--m_freeSlots;
			return BaseHandle( index, entry->m_counter );
		}

		void HandleManager::Update( void* ptr, BaseHandle& handle )
		{
			assert( handle.IsValid() );

			HandleEntry* entry = m_array.data() + handle.m_index;

			assert( entry->m_allocated );

			entry->m_ptr = ptr;
		}

		void HandleManager::Remove( const BaseHandle& handle )
		{
			HandleEntry* entry = m_array.data() + handle.m_index;

			assert( entry->m_allocated );

			entry->m_counter++;
			entry->m_allocated = false;

			// Push removed slot onto freeList
			entry->m_nextFreeIndex = m_freeList;
			m_freeList = handle.m_index;

			++m_freeSlots;
		}

		void* HandleManager::Get( const BaseHandle& handle ) const
		{
			const HandleEntry* entry = m_array.data() + handle.m_index;

			if( entry->m_counter == handle.m_counter && entry->m_allocated )
				return entry->m_ptr;

			return NULL;
		}

		bool HandleManager::IsValid( const BaseHandle& handle ) const
		{
			const HandleEntry* entry = m_array.data() + handle.m_index;

			if( entry->m_counter == handle.m_counter && entry->m_allocated )
				return true;

			return false;
		}

		void HandleManager::Replace( void* ptr, BaseHandle& handle )
		{
			HandleEntry* entry = m_array.data() + handle.m_index;

			// Increment the uid counter to signify a new handle at
			// this entry
			if( entry->m_allocated )
			{
				entry->m_counter++;
				handle.m_counter = entry->m_counter;
			}

			entry->m_ptr = ptr;
		}

		unsigned HandleManager::FreeSlots() const
		{
			return m_freeSlots;
		}
	}
}