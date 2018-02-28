#include "ObjectAllocator.h"

#include <cstring>
#include <assert.h>
#include <malloc.h>

namespace Reflex
{
	namespace Core
	{
		ObjectAllocator::ObjectAllocator( unsigned objectSize, unsigned numElements )
			: m_array( malloc( objectSize * numElements ) )
			, m_objectSize( objectSize )
			, m_size( 0U )
			, m_capacity( numElements )
			, m_arrayGrew( false )
		{

		}

		void* ObjectAllocator::Allocate()
		{
			if( m_size == m_capacity )
				Grow();

			return ( char * )m_array + m_size++ * m_objectSize;
		}

		void* ObjectAllocator::Release( void* memory )
		{
			if( --m_size )
			{
				unsigned index = GetIndex( memory );
				Move( index, m_size );
				return ( char * )m_array + index * m_objectSize;
			}

			return nullptr;
		}

		void ObjectAllocator::Grow()
		{
			m_capacity = m_capacity ? ( m_capacity * 2 + 10 ) : 4;
			m_arrayGrew = true;

			GrowInteral();
		}

		void ObjectAllocator::GrowInteral()
		{
			void* newArray = malloc( m_objectSize * m_capacity );

			std::memcpy( newArray, m_array, m_size * m_objectSize );

			free( m_array );

			m_array = newArray;
		}

		void ObjectAllocator::Swap( unsigned a, unsigned b )
		{
			assert( a < m_size );
			assert( b < m_size );

			void* t = alloca( m_objectSize );
			void* A = ( char * )m_array + a * m_objectSize;
			void* B = ( char * )m_array + b * m_objectSize;
			std::memcpy( t, A, m_objectSize );
			std::memcpy( A, B, m_objectSize );
			std::memcpy( B, t, m_objectSize );
		}

		void ObjectAllocator::Move( unsigned a, unsigned b )
		{
			void *A = ( char * )m_array + a * m_objectSize;
			void *B = ( char * )m_array + b * m_objectSize;
			std::memcpy( A, B, m_objectSize );
		}

		void ObjectAllocator::Shrink()
		{
			m_capacity = m_size;

			GrowInteral();
		}

		void ObjectAllocator::ShrinkTo( unsigned numElements )
		{
			m_capacity = ( numElements > m_size ? numElements : m_size );

			GrowInteral();
		}

		void ObjectAllocator::ShrinkBy( unsigned numElements )
		{
			const unsigned diff = m_capacity - numElements;
			m_capacity = ( diff > m_size ? diff : m_size );

			GrowInteral();
		}

		void* ObjectAllocator::GetData( unsigned index ) const
		{
			assert( index < m_size );
			return ( char* )m_array + index * m_objectSize;
		}

		void* ObjectAllocator::operator[]( unsigned index )
		{
			return GetData( index );
		}

		const void* ObjectAllocator::operator[]( unsigned index ) const
		{
			return GetData( index );
		}

		unsigned ObjectAllocator::Size() const
		{
			return m_size;
		}

		unsigned ObjectAllocator::Capacity() const
		{
			return m_capacity;
		}

		unsigned ObjectAllocator::GetIndex( void* data ) const
		{
			return ( ( char* )data - ( char* )m_array ) / m_objectSize;
		}

		bool ObjectAllocator::Grew( void ) const
		{
			return m_arrayGrew;
		}

		void ObjectAllocator::ClearGrewFlag()
		{
			m_arrayGrew = false;
		}
	}
}