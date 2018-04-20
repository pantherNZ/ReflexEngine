#include "TileMap.h"

#include "TransformComponent.h"
#include "Object.h"

TODO( "Allow TileMap work when a pos outside the bounds is entered - dynamically resize the array" )

namespace Reflex
{
	namespace Core
	{
		TileMap::TileMap( const sf::FloatRect& worldBounds, const unsigned tileMapGridSize /*= 0U*/ )
			: m_worldBounds( worldBounds )
			, m_tileMapGridSize( tileMapGridSize )
		{
		}

		TileMap::TileMap( const sf::FloatRect& worldBounds, const SpacialStorageType type, const unsigned storageSize, const unsigned tileMapGridSize /*= 0U*/ )
			: m_worldBounds( worldBounds )
			, m_tileMapGridSize( tileMapGridSize )
		{
			Reset( storageSize, false );
		}

		void TileMap::Insert( ObjectHandle obj )
		{
			if( obj )
			{
				const auto id = GetID( obj );
				auto& bucket = m_spacialHashMap[id];
				bucket.insert( obj );
			}
		}

		void TileMap::Insert( ObjectHandle obj, const sf::Vector2f& topLeft, const sf::Vector2f& botRight )
		{
			if( obj )
			{
				const auto ids = GetID( topLeft, botRight );

				for( auto& id : ids )
				{
					auto& bucket = m_spacialHashMap[id];
					bucket.insert( obj );
				}
			}
		}

		void TileMap::Remove( ObjectHandle obj )
		{
			if( obj )
			{
				const auto id = GetID( obj );
				auto& bucket = m_spacialHashMap[id];
				const auto found = bucket.find( obj );
				assert( found != bucket.end() );
				if( found != bucket.end() )
					bucket.erase( found );
			}
		}

		void TileMap::RemoveByID( ObjectHandle obj, const unsigned id )
		{
			if( obj )
			{
				auto& bucket = m_spacialHashMap[id];
				const auto found = bucket.find( obj );
				assert( found != bucket.end() );
				if( found != bucket.end() )
					bucket.erase( found );
			}
		}

		void TileMap::GetNearby( ObjectHandle obj, std::vector< ObjectHandle >& out ) const
		{
			ForEachNearby( obj, [&out]( const ObjectHandle& obj )
			{
				out.push_back( obj );
			} );
		}

		void TileMap::GetNearby( const sf::Vector2f& position, std::vector< ObjectHandle >& out ) const
		{
			ForEachNearby( position, [&out]( const ObjectHandle& obj )
			{
				out.push_back( obj );
			} );
		}

		void TileMap::GetNearby( ObjectHandle obj, const sf::Vector2f& topLeft, const sf::Vector2f& botRight, std::vector< ObjectHandle >& out ) const
		{
			ForEachNearby( obj, topLeft, botRight, [&out]( const ObjectHandle& obj )
			{
				out.push_back( obj );
			} );
		}

		void TileMap::Reset( const bool shouldRePopulate /*= false*/ )
		{
			m_spacialHashMap.empty();
			m_spacialHashMapWidth = ( unsigned )std::ceil( m_worldBounds.width / m_storageSize );
			m_spacialHashMapHeight = ( unsigned )std::ceil( m_worldBounds.height / m_storageSize );
			m_spacialHashMap.resize( m_spacialHashMapWidth * m_spacialHashMapHeight );

			TODO( "Implement shouldRePopulate system from world" );
		}

		void TileMap::Reset( const unsigned spacialHashMapGridSize, const bool shouldRePopulate /*= false*/ )
		{
			m_storageSize = spacialHashMapGridSize;
			Reset( shouldRePopulate );

			TODO( "Implement shouldRePopulate system from world" );
		}

		unsigned TileMap::GetID( const ObjectHandle obj ) const
		{
			assert( obj );
			return GetID( obj->GetComponent< Reflex::Components::Transform >()->getPosition() );
		}

		unsigned TileMap::GetID( const sf::Vector2f& position ) const
		{
			const auto loc = Hash( position );
			return loc.y * m_spacialHashMapWidth + loc.x;
		}

		std::vector< unsigned > TileMap::GetID( const sf::Vector2f& topLeft, const sf::Vector2f& botRight ) const
		{
			const auto locTopLeft = Hash( topLeft );
			const auto locBotRight = Hash( botRight );
			std::vector< unsigned > ids;

			for( int x = locTopLeft.x; x <= locBotRight.x; ++x )
				for( int y = locTopLeft.y; y <= locBotRight.y; ++y )
					ids.push_back( y * m_spacialHashMapWidth+ x );

			return std::move( ids );
		}

		sf::Vector2i TileMap::Hash( const sf::Vector2f& position ) const
		{
			return sf::Vector2i( int( position.x / m_storageSize ), int( position.y / m_storageSize ) );
		}
	}
}