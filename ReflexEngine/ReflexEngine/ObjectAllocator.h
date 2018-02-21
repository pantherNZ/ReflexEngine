#pragma once

namespace Reflex
{
	namespace Core
	{
		class ObjectAllocator
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

			// Operator overloads for accessing the data
			void* operator[]( unsigned index );
			const void* operator[]( unsigned index ) const;

		private:
			void Grow();
			void GrowInteral();
			void Move( unsigned a, unsigned b );

			// Remove
			ObjectAllocator() = delete;
			ObjectAllocator( const ObjectAllocator& ) = delete;
			ObjectAllocator& operator=( const ObjectAllocator& ) = delete;

		private:
			void* mArray;
			unsigned mObjectSize;
			unsigned mSize;
			unsigned mCapacity;
			bool mArrayGrew;
		};
	}
}