#pragma once

#include "Precompiled.h"
#include "Handle.h"

#include <array>

namespace Reflex
{
	namespace Core
	{
		class HandleManager : private sf::NonCopyable
		{
		public:
			HandleManager();

			void Clear();

			BaseHandle Insert( void* ptr );

			template< class T >
			Handle< T > Insert( void* ptr );

			void Replace( void* ptr, BaseHandle& handle );

			void Update( void* ptr, BaseHandle& handle );

			template< class T >
			void Update( T* ptr );

			void Remove( const BaseHandle& handle );

			void* Get( const BaseHandle& handle ) const;

			template< class T >
			T* GetAs( const Handle< T >& handle ) const;

			bool IsValid( const BaseHandle& handle ) const;

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
			return Handle< T >( Insert( ptr ) );
		}

		template< class T >
		void HandleManager::Update( T* ptr )
		{
			Update( ptr, ptr->m_self );
		}

		template< class T >
		T* HandleManager::GetAs( const Handle< T >& handle ) const
		{
			const HandleEntry* entry = m_array.data() + handle.m_index;
			
			if( entry->m_counter == handle.m_counter && entry->m_allocated )
				return ( T* )entry->m_ptr;

			return NULL;
		}
	}
}