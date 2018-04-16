#pragma once

#include "Common.h"
#include "Component.h"

namespace Reflex
{
	namespace Core
	{
		class TileMap : sf::NonCopyable
		{
		public:
			explicit TileMap( const sf::FloatRect& worldBounds, const unsigned spacialHashMapGridSize, const unsigned tileMapGridSize = 0U );

			void Insert( ObjectHandle obj );
			void Insert( ObjectHandle obj, const sf::Vector2f& topLeft, const sf::Vector2f& botRight );
			void Remove( ObjectHandle obj );

			void GetNearby( ObjectHandle obj, std::vector< ObjectHandle >& out ) const;
			void GetNearby( ObjectHandle obj, const sf::Vector2f& topLeft, const sf::Vector2f& botRight, std::vector< ObjectHandle >& out ) const;

			template< typename Func >
			void ForEachNearby( ObjectHandle obj, Func f ) const;

			template< typename Func >
			void ForEachNearby( ObjectHandle obj, const sf::Vector2f& topLeft, const sf::Vector2f& botRight, Func f ) const;

			void Reset( const bool shouldRePopulate = false );
			void Reset( const unsigned spacialHashMapGridSize, const bool shouldRePopulate = false );

		private:
			std::vector< unsigned > GetIDForObject( const sf::Vector2f& topLeft, const sf::Vector2f& botRight ) const;
			unsigned GetIDForObject( ObjectHandle obj ) const;
			sf::Vector2i Hash( const sf::Vector2f& loc ) const;

		private:
			const sf::FloatRect m_worldBounds;
			unsigned m_spacialHashMapGridSize = 0U;
			unsigned m_tileMapGridSize = 0U;
			unsigned m_spacialHashMapWidth = 0U;
			unsigned m_spacialHashMapHeight = 0U;
			std::vector< std::unordered_set< ObjectHandle > > m_spacialHashMap;
		};

		template< typename Func >
		void TileMap::ForEachNearby( ObjectHandle obj, Func f ) const
		{
			if( !obj )
				return;

			const auto id = GetIDForObject( obj );
			auto& bucket = m_spacialHashMap[id];

			for( auto& item : bucket )
				if( item != obj )
					f( obj );
		}

		template< typename Func >
		void TileMap::ForEachNearby( ObjectHandle obj, const sf::Vector2f& topLeft, const sf::Vector2f& botRight, Func f ) const
		{
			if( !obj )
				return;

			const auto ids = GetIDForObject( topLeft, botRight );

			for( auto& id : ids )
			{
				auto& bucket = m_spacialHashMap[id];

				for( auto& item : bucket )
					if( item != obj )
						f( obj );
			}

		}
	}
}