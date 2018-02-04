#pragma once

#include "Common.h"

namespace Reflex
{
	namespace Core
	{
		enum class ResourceID : unsigned short
		{
			TEST = 0,
		};

		template< typename Resource >
		class ResouceManager
		{
		public:
			// Resource loading
			void LoadResource( const ResourceID id, const std::string& filename );

			// Resource loading with a second parameter (for shaders, or textures that specify a bounding rect etc.)
			template< typename Parameter >
			void LoadResource( const ResourceID id, const std::string& filename, const Parameter& secondParam );

			// Fetches a resource from the map
			const Resource& GetResource( const ResourceID id ) const;

		private:
			// Private helper function to inset a new resource into the map and do error checking
			void InsertResource( std::unique_ptr< Resource > newResource );

			std::map< const ResourceID, std::unique_ptr< Resource > > m_resourceMap;
		};
	}
}