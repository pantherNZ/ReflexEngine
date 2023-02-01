#pragma once

namespace Reflex
{
	namespace Core
	{
		template< class T >
		class ArrayIterator : public std::iterator< std::random_access_iterator_tag, T >
		{
		public:
			ArrayIterator();
			ArrayIterator( T* data );

			T& operator*( void ) const;
			T* operator->( void ) const;
			ArrayIterator& operator++( void );
			ArrayIterator& operator--( void );
			ArrayIterator operator++( int );
			ArrayIterator operator--( int );
			ArrayIterator operator+( int x ) const;
			ArrayIterator operator-( int x ) const;
			unsigned operator-( const ArrayIterator& rhs ) const;
			ArrayIterator& operator+=( int x );
			ArrayIterator& operator-=( int x );
			bool operator<( const ArrayIterator& rhs ) const;
			bool operator==( const ArrayIterator& rhs ) const;
			bool operator!=( const ArrayIterator& rhs ) const;

		private:
			T* m_data;
		};

		template< typename T >
		ArrayIterator< T >::ArrayIterator()
			: m_data( nullptr )
		{
		}

		template< typename T >
		ArrayIterator< T >::ArrayIterator( T* data )
			: m_data( data )
		{
		}

		template< typename T >
		T& ArrayIterator< T >::operator*() const
		{
			return *m_data;
		}

		template< typename T >
		T *ArrayIterator<T>::operator->() const
		{
			return m_data;
		}

		template< typename T >
		typename ArrayIterator<T>& ArrayIterator<T>::operator++()
		{
			++m_data;
			return *this;
		}

		template< typename T >
		typename ArrayIterator<T>& ArrayIterator<T>::operator--()
		{
			--m_data;
			return *this;
		}

		template< typename T >
		typename ArrayIterator<T> ArrayIterator<T>::operator++( int )
		{
			return ArrayIterator( m_data++ );
		}

		template< typename T >
		typename ArrayIterator<T> ArrayIterator<T>::operator--( int )
		{
			return ArrayIterator( m_data-- );
		}

		template< typename T >
		bool ArrayIterator<T>::operator==( const ArrayIterator &rhs ) const
		{
			return m_data == rhs.m_data;
		}

		template< typename T >
		bool ArrayIterator< T >::operator!=( const ArrayIterator &rhs ) const
		{
			return m_data != rhs.m_data;
		}

		template< typename T >
		ArrayIterator< T > ArrayIterator<T>::operator+( int x ) const
		{
			return ArrayIterator( m_data + x );
		}

		template< typename T >
		typename ArrayIterator<T> ArrayIterator<T>::operator-( int x ) const
		{
			return ArrayIterator( m_data - x );
		}

		template< typename T >
		unsigned ArrayIterator<T>::operator-( const ArrayIterator& rhs ) const
		{
			return m_data - rhs.m_data;
		}

		template< typename T >
		typename ArrayIterator<T>& ArrayIterator<T>::operator+=( int x )
		{
			m_data += x;
			return *this;
		}

		template< typename T >
		typename ArrayIterator<T>& ArrayIterator<T>::operator-=( int x )
		{
			m_data -= x;
			return *this;
		}

		template< typename T >
		bool ArrayIterator<T>::operator<( const ArrayIterator& rhs ) const
		{
			return m_data < rhs.m_data;
		}
	}
}