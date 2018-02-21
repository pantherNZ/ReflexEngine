#include "ObjectAllocator.h"

#include <cstring>
#include <assert.h>
#include <malloc.h>

namespace Reflex
{
	namespace Core
	{
		ObjectAllocator::ObjectAllocator( unsigned objectSize, unsigned numElements )
			: mArray( malloc( objectSize * numElements ) )
			, mObjectSize( objectSize )
			, mSize( 0U )
			, mCapacity( numElements )
			, mArrayGrew( false )
		{

		}

		void* ObjectAllocator::Allocate()
		{
			if( mSize == mCapacity )
				Grow();

			return ( char * )mArray + mSize++ * mObjectSize;
		}

		void* ObjectAllocator::Release( void* memory )
		{
			if( --mSize )
			{
				unsigned index = GetIndex( memory );
				Move( index, mSize );
				return ( char * )mArray + index * mObjectSize;
			}

			return nullptr;
		}

		void ObjectAllocator::Grow()
		{
			mCapacity = mCapacity ? ( mCapacity * 2 + 10 ) : 4;
			mArrayGrew = true;

			GrowInteral();
		}

		void ObjectAllocator::GrowInteral()
		{
			void* newArray = malloc( mObjectSize * mCapacity );

			std::memcpy( newArray, mArray, mSize * mObjectSize );

			free( mArray );

			mArray = newArray;
		}

		void ObjectAllocator::Swap( unsigned a, unsigned b )
		{
			assert( a < mSize );
			assert( b < mSize );

			void* t = alloca( mObjectSize );
			void* A = ( char * )mArray + a * mObjectSize;
			void* B = ( char * )mArray + b * mObjectSize;
			std::memcpy( t, A, mObjectSize );
			std::memcpy( A, B, mObjectSize );
			std::memcpy( B, t, mObjectSize );
		}

		void ObjectAllocator::Move( unsigned a, unsigned b )
		{
			void *A = ( char * )mArray + a * mObjectSize;
			void *B = ( char * )mArray + b * mObjectSize;
			std::memcpy( A, B, mObjectSize );
		}

		void ObjectAllocator::Shrink()
		{
			mCapacity = mSize;

			GrowInteral();
		}

		void ObjectAllocator::ShrinkTo( unsigned numElements )
		{
			mCapacity = ( numElements > mSize ? numElements : mSize );

			GrowInteral();
		}

		void ObjectAllocator::ShrinkBy( unsigned numElements )
		{
			const unsigned diff = mCapacity - numElements;
			mCapacity = ( diff > mSize ? diff : mSize );

			GrowInteral();
		}

		void* ObjectAllocator::operator[]( unsigned index )
		{
			assert( index < mSize );
			return ( char* )mArray + index * mObjectSize;
		}

		const void* ObjectAllocator::operator[]( unsigned index ) const
		{
			assert( index < mSize );
			return ( char* )mArray + index * mObjectSize;
		}

		unsigned ObjectAllocator::Size() const
		{
			return mSize;
		}

		unsigned ObjectAllocator::Capacity() const
		{
			return mCapacity;
		}

		unsigned ObjectAllocator::GetIndex( void* data ) const
		{
			return ( ( char* )data - ( char* )mArray ) / mObjectSize;
		}

		bool ObjectAllocator::Grew( void ) const
		{
			return mArrayGrew;
		}

		void ObjectAllocator::ClearGrewFlag()
		{
			mArrayGrew = false;
		}
	}
}