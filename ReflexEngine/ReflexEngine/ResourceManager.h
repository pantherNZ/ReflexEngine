#pragma once

#include "Common.h"

namespace Reflex
{
	enum class ResourceID : unsigned short;

	namespace Core
	{
		template< typename Resource >
		class ResouceManager
		{
		public:
			// Resource loading
			Resource& LoadResource( const ResourceID id, const std::string& filename );

			// Resource loading with a second parameter (for shaders, or textures that specify a bounding rect etc.)
			template< typename Parameter >
			Resource& LoadResource( const ResourceID id, const std::string& filename, const Parameter& secondParam );

			// Fetches a resource from the map
			const Resource& GetResource( const ResourceID id ) const;

		private:
			// Private helper function to insert a new resource into the map and do error checking
			Resource& InsertResource( const ResourceID id, const std::string& filename, std::unique_ptr< Resource > newResource );

			std::map< const ResourceID, std::unique_ptr< Resource > > m_resourceMap;
		};
	}
}

#include "ResourceManger.inl"