#pragma once

#include "ResourceManager.h"

namespace Reflex
{
	namespace Core
	{
		template< typename Resource >
		Resource& ResouceManager< Resource >::LoadResource( const ResourceID id, const std::string& filename )
		{
			auto newResource = std::make_unique< Resource >();

			if( !newResource->loadFromFile( filename ) )
				throw std::runtime_error( "ResouceManager::LoadResource | Failed to load " + filename );

			return InsertResource( id, filename, std::move( newResource ) );
		}

		template< typename Resource >
		template< typename Parameter >
		Resource& ResouceManager< Resource >::LoadResource( const ResourceID id, const std::string& filename, const Parameter& secondParam )
		{
			auto newResource = std::make_unique< Resource >();

			if( !newResource->loadFromFile( filename, secondParam ) )
				throw std::runtime_error( "ResouceManager::LoadResource | Failed to load " + filename );

			return InsertResource( id, filename, std::move( newResource ) );
		}

		template< typename Resource >
		const Resource& ResouceManager< Resource >::GetResource( const ResourceID id ) const
		{
			auto found = m_resourceMap.find( id );

			if( found == m_resourceMap.end() )
				LOG_CRIT( "ResouceManager::GetResource | Resource doesn't exist " + std::to_string( ( int )id ) );

			return *found->second;
		}

		template< typename Resource >
		Resource& ResouceManager< Resource >::InsertResource( const ResourceID id, const std::string& filename, std::unique_ptr< Resource > newResource )
		{
			auto inserted = m_resourceMap.insert( std::make_pair( id, std::move( newResource ) ) );

			if( !inserted.second )
				LOG_CRIT( "ResouceManager::LoadResource | Resource already loaded " + filename );

			auto& resource_iter = *inserted.first;
			return *resource_iter.second;
		}
	}
}