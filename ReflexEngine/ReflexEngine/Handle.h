#pragma once

namespace Reflex
{
	namespace Core
	{
		class HandleManager;

		class BaseHandle
		{
		public:
			BaseHandle();
			BaseHandle( uint32_t index, uint32_t counter );
			BaseHandle( uint32_t handle );

			explicit operator bool() const;
			operator unsigned() const;
			bool IsValid() const;

			// Members
			uint32_t m_index : 16;
			uint32_t m_counter : 16;

			static BaseHandle null;
			static class HandleManager* s_handleManager;
		};

		template< class T >
		class Handle : public BaseHandle
		{
		public:
			Handle() : BaseHandle() { }
			Handle( const BaseHandle& handle );
			T* Get() const;
			T* operator ->() const;
		};

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