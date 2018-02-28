#pragma once

#include "Common.h"
#include "ObjectIterator.h"

namespace Reflex
{
	namespace Core
	{
		class ObjectAllocator : private sf::NonCopyable
		{
		public:
			ObjectAllocator( unsigned objectSize, unsigned numElements );

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
			bool Grew() const;
			void ClearGrewFlag();
			void* GetData( unsigned index ) const;

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
			void Move( unsigned a, unsigned b );

			ObjectAllocator() = delete;

		private:
			void* m_array;
			unsigned m_objectSize;
			unsigned m_size;
			unsigned m_capacity;
			bool m_arrayGrew;
		};
	}
}