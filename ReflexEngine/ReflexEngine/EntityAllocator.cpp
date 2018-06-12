#include "EntityAllocator.h"

#include <cstring>
#include <assert.h>
#include <malloc.h>

namespace Reflex
{
	namespace Core
	{
		EntityAllocator::EntityAllocator( unsigned objectSize, unsigned numElements )
			: m_array( malloc( objectSize * numElements ) )
			, m_objectSize( objectSize )
			, m_size( 0U )
			, m_capacity( numElements )
		{

		}

		EntityAllocator::~EntityAllocator()
		{
			free( m_array );
		}

		bool EntityAllocator::PreAllocate()
		{
			if( m_size == m_capacity )
			{
				Grow();
				return true;
			}

			return false;
		}

		void* EntityAllocator::Allocate()
		{
			if( m_size == m_capacity )
				Grow();

			return ( char * )m_array + m_size++ * m_objectSize;
		}

		void* EntityAllocator::Release( void* memory )
		{
			if( --m_size )
			{
				unsigned index = GetIndex( memory );

				if( index == m_size )
					return nullptr;

				Move( index, m_size );
				return ( char * )m_array + index * m_objectSize;
			}

			return nullptr;
		}

		void EntityAllocator::Swap( unsigned a, unsigned b )
		{
			if( a == b )
				return;

			assert( a < m_size );
			assert( b < m_size );

			void* t = malloc( m_objectSize );
			void* A = ( char * )m_array + a * m_objectSize;
			void* B = ( char * )m_array + b * m_objectSize;
			std::memcpy( t, A, m_objectSize );
			std::memcpy( A, B, m_objectSize );
			std::memcpy( B, t, m_objectSize );
		}

		void EntityAllocator::Shrink()
		{
			m_capacity = m_size;

			GrowInteral();
		}

		void EntityAllocator::ShrinkTo( unsigned numElements )
		{
			m_capacity = ( numElements > m_size ? numElements : m_size );

			GrowInteral();
		}

		void EntityAllocator::ShrinkBy( unsigned numElements )
		{
			const unsigned diff = m_capacity - numElements;
			m_capacity = ( diff > m_size ? diff : m_size );

			GrowInteral();
		}

		void* EntityAllocator::operator[]( unsigned index )
		{
			return GetData( index );
		}

		const void* EntityAllocator::operator[]( unsigned index ) const
		{
			return GetData( index );
		}

		unsigned EntityAllocator::Size() const
		{
			return m_size;
		}

		unsigned EntityAllocator::Capacity() const
		{
			return m_capacity;
		}

		unsigned EntityAllocator::GetIndex( void* data ) const
		{
			return ( ( char* )data - ( char* )m_array ) / m_objectSize;
		}

		void* EntityAllocator::GetData( unsigned index ) const
		{
			assert( index < m_size );
			return ( char* )m_array + index * m_objectSize;
		}

		bool EntityAllocator::Grew() const
		{
			return m_arrayGrew;
		}

		void EntityAllocator::ClearGrewFlag()
		{
			m_arrayGrew = false;
		}

		void EntityAllocator::Grow()
		{
			m_capacity = m_capacity ? ( m_capacity * 2 + 10 ) : 4;
			m_arrayGrew = true;

			GrowInteral();
		}

		void EntityAllocator::GrowInteral()
		{
			void* newArray = malloc( m_objectSize * m_capacity );

			std::memcpy( newArray, m_array, m_size * m_objectSize );

			free( m_array );

			m_array = newArray;
		}

		void EntityAllocator::Move( unsigned dest, unsigned src )
		{
			if( dest == src )
				return;

			void* destPtr = ( char * )m_array + dest * m_objectSize;
			void* srcPtr = ( char * )m_array + src * m_objectSize;
			std::memcpy( destPtr, srcPtr, m_objectSize );
		}
	}
}