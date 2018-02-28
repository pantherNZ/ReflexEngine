#pragma once

#include "Common.h"
#include "Handle.h"

namespace Reflex
{
	namespace Core
	{
		class HandleManager : private sf::NonCopyable
		{
		public:
			HandleManager();

			void Clear();

			template< class T >
			Handle< T > Insert( void* ptr );

			template< class T >
			void Replace( void* ptr, Handle< T >& handle );

			template< class T >
			void Update( void* ptr, Handle< T >& handle );

			template< class T >
			void Remove( const Handle< T >& handle );

			template< class T >
			void* Get( const Handle< T >& handle ) const;

			template< class T >
			bool IsValid( const Handle< T >& handle ) const;

			template< class T >
			T* GetAs( const Handle< T >& handle ) const;

			unsigned FreeSlots() const;

		private:
			struct HandleEntry
			{
				HandleEntry();
				HandleEntry( unsigned nextFreeIndex );

				unsigned m_nextFreeIndex : 16;
				unsigned m_counter : 16;
				unsigned m_endOfList : 1;
				unsigned m_allocated : 1;
				void* m_ptr;
			};

			unsigned m_freeSlots;
			unsigned m_freeList;

			enum { m_MaxEntries = 16384 };
			std::array< HandleEntry, m_MaxEntries > m_array;
		};

		// Template definitions
		template< class T >
		Handle< T > HandleManager::Insert( void* ptr )
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
			return Handle< T >( index, entry->m_counter );
		}

		template< class T >
		void HandleManager::Replace( void* ptr, Handle< T >& handle )
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

		template< class T >
		void HandleManager::Update( void* ptr, Handle< T >& handle )
		{
			assert( handle.IsValid() );

			HandleEntry* entry = m_array.data() + handle.m_index;

			assert( entry->m_allocated );

			entry->m_ptr = ptr;
		}

		template< class T >
		void HandleManager::Remove( const Handle< T >& handle )
		{
			HandleEntry* entry = m_array.data() + handle.m_index;

			// Internal bug if fires
			assert( entry->m_allocated );

			entry->m_counter++;
			entry->m_allocated = false;

			// Push removed slot onto freeList
			entry->m_nextFreeIndex = m_freeList;
			m_freeList = handle.m_index;

			++m_freeSlots;
		}

		template< class T >
		void* HandleManager::Get( const Handle< T >& handle ) const
		{
			const HandleEntry* entry = m_array.data() + handle.m_index;

			if( entry->m_counter == handle.m_counter && entry->m_allocated )
				return entry->m_ptr;

			return NULL;
		}

		template< class T >
		bool HandleManager::IsValid( const Handle< T >& handle ) const
		{
			const HandleEntry* entry = m_array.data() + handle.m_index;

			if( entry->m_counter == handle.m_counter && entry->m_allocated )
				return true;

			return false;
		}

		template< class T >
		T* HandleManager::GetAs( const Handle< T >& handle ) const
		{
			const HandleEntry* entry = m_array + handle.m_index;

			if( entry->m_counter == handle.m_counter && entry->m_allocated )
				return ( T* )entry->m_ptr;

			return NULL;
		}
	}
}