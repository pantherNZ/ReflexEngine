#include "TileMap.h"

#include "TransformComponent.h"
#include "Object.h"

namespace Reflex
{
	namespace Core
	{
		TileMap::TileMap( const sf::FloatRect& worldBounds, const unsigned spacialHashMapGridSize, const unsigned tileMapGridSize )
			: m_worldBounds( worldBounds )
			, m_tileMapGridSize( tileMapGridSize )
		{
			Reset( spacialHashMapGridSize, false );
		}

		void TileMap::Insert( ObjectHandle obj )
		{
			if( obj )
			{
				const auto id = GetIDForObject( obj );
				auto& bucket = m_spacialHashMap[id];
				bucket.insert( obj );
			}
		}

		void TileMap::Insert( ObjectHandle obj, const sf::Vector2f& topLeft, const sf::Vector2f& botRight )
		{
			if( obj )
			{
				const auto ids = GetIDForObject( topLeft, botRight );

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
				const auto id = GetIDForObject( obj );
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
			m_spacialHashMapWidth = ( unsigned )std::ceil( m_worldBounds.width / m_spacialHashMapGridSize );
			m_spacialHashMapHeight = ( unsigned )std::ceil( m_worldBounds.height / m_spacialHashMapGridSize );
			m_spacialHashMap.resize( m_spacialHashMapWidth * m_spacialHashMapHeight );

			TODO( "Implement shouldRePopulate system from world" );
		}

		void TileMap::Reset( const unsigned spacialHashMapGridSize, const bool shouldRePopulate /*= false*/ )
		{
			m_spacialHashMapGridSize = spacialHashMapGridSize;
			Reset( shouldRePopulate );

			TODO( "Implement shouldRePopulate system from world" );
		}

		unsigned TileMap::GetIDForObject( ObjectHandle obj ) const
		{
			assert( obj );
			const auto loc = Hash( obj->GetComponent< Reflex::Components::TransformComponent >()->getPosition() );
			return loc.y * m_spacialHashMapHeight + loc.x;
		}

		std::vector< unsigned > TileMap::GetIDForObject( const sf::Vector2f& topLeft, const sf::Vector2f& botRight ) const
		{
			const auto locTopLeft = Hash( topLeft );
			const auto locBotRight = Hash( botRight );
			std::vector< unsigned > ids;

			for( int x = locTopLeft.x; x <= locBotRight.x; ++x )
				for( int y = locTopLeft.y; y <= locBotRight.y; ++y )
					ids.push_back( y * m_spacialHashMapHeight + x );

			return std::move( ids );
		}

		sf::Vector2i TileMap::Hash( const sf::Vector2f& loc ) const
		{
			return sf::Vector2i( int( loc.x / m_spacialHashMapGridSize ), int( loc.y / m_spacialHashMapGridSize ) );
		}
	}
}