#pragma once

#include "Precompiled.h"
#include "EntityIterator.h"

namespace Reflex
{
	namespace Core
	{
		class EntityAllocator : private sf::NonCopyable
		{
		public:
			EntityAllocator( unsigned objectSize, unsigned numElements );
			~EntityAllocator();

			// Allocates new capacity if required, but does not actually create a new object*, returns whether new space was allocated
			bool PreAllocate();

			// Allocate space and return index
			void* Allocate();

			// Move rightmost element into deleted slot, does not placement delete
			// Returns pointer to the moved element (was rightmost element before free)
			void* Release( void* memory );

			// Swap memory of two indices
			void Swap( unsigned a, unsigned b );

			// Reduce capacity
			void Shrink();
			void ShrinkTo( unsigned numElements );
			void ShrinkBy( unsigned numElements );

			// Helper functions
			unsigned Size() const;
			unsigned Capacity() const;
			unsigned GetIndex( void* data ) const;
			void* GetData( unsigned index ) const;
			bool Grew() const;
			void ClearGrewFlag();

			// Operator overloads for accessing the data
			void* operator[]( unsigned index );
			const void* operator[]( unsigned index ) const;

			template< class T >
			inline typename ArrayIterator< T > begin() { return ArrayIterator< T >( ( T* )m_array ); }
			template< class T >
			inline typename ArrayIterator< T > end() { return ArrayIterator< T >( ( T* )m_array + m_size ); }

		private:
			void Grow();
			void GrowInteral();
			void Move( unsigned dest, unsigned src );

			EntityAllocator() = delete;

		private:
			void* m_array = nullptr;
			unsigned m_objectSize = 0U;
			unsigned m_size = 0U;
			unsigned m_capacity = 0U;
			bool m_arrayGrew = false;
		};
	}
}