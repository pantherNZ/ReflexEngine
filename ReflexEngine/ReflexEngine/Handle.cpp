#include "HandleManager.h"
#include "Object.h"
#include "Component.h"

namespace Reflex
{
	namespace Core
	{
		// Statics
		HandleManager* BaseHandle::s_handleManager = nullptr;
		BaseHandle BaseHandle::null;

		BaseHandle::BaseHandle()
			: m_index( -1 )
			, m_counter( -1 )
		{
		}

		BaseHandle::BaseHandle( uint32_t index, uint32_t counter )
			: m_index( index )
			, m_counter( counter )
		{
		}

		BaseHandle::BaseHandle( uint32_t handle )
		{
			// Cast this to an unsigned pointer, deference and then set this to the handle (which will set each member respectively)
			*( ( uint32_t* )this ) = handle;
		}

		BaseHandle::operator bool() const
		{
			return IsValid();
		}

		BaseHandle::operator unsigned() const
		{
			return m_counter << 16 | m_index;
		}

		bool BaseHandle::IsValid() const
		{
			return m_index != null.m_index && m_counter != null.m_counter;
		}
	}
}