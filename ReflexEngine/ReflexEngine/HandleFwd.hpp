#pragma once

#include "Handle.h"
#include "HandleManager.h"

#include <type_traits>

namespace Reflex
{
	namespace Core
	{
		// Forward declaration of common handle types
		typedef Handle< class Object > ObjectHandle;

		// Template functions
		template< class T >
		Handle< T >::Handle( const BaseHandle& handle )
			: BaseHandle( handle.m_index, handle.m_counter )
		{

		}

		template< class T >
		T* Handle< T >::Get() const
		{
			return s_handleManager->GetAs< T >( *this );
		}

		template< class T >
		T* Handle< T >::operator ->() const
		{
			return Get();
		}
	}
}

// Hash function for handles allows it to be used with hash maps such as std::unordered_map etc.
namespace std
{
	template<>
	struct hash< Reflex::Core::BaseHandle >
	{
		std::size_t operator()( const Reflex::Core::BaseHandle& k ) const
		{
			return hash< uint32_t >()( k.m_index ) ^ ( hash< uint32_t >()( k.m_counter ) >> 1 );
		}
	};

	template< class T >
	struct hash< Reflex::Core::Handle< T > >
	{
		std::size_t operator()( const Reflex::Core::Handle< T >& k ) const
		{
			return hash< uint32_t >()( k.m_index ) ^ ( hash< uint32_t >()( k.m_counter ) >> 1 );
		}
	};
}