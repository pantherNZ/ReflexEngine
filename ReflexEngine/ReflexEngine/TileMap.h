#pragma once

#include "Precompiled.h"

#include "TransformComponent.h"

namespace Reflex
{
	namespace Core
	{
		enum SpacialStorageType
		{
			SpacialHashMap,
			QuadTree,
		};

		class TileMap : sf::NonCopyable
		{
			friend class Reflex::Components::Transform;

		public:
			explicit TileMap( const sf::FloatRect& worldBounds, const unsigned tileMapGridSize = 0U );
			explicit TileMap( const sf::FloatRect& worldBounds, const SpacialStorageType type, const unsigned storageSize, const unsigned tileMapGridSize = 0U );

			void Insert( ObjectHandle obj );
			void Insert( ObjectHandle obj, const sf::Vector2f& topLeft, const sf::Vector2f& botRight );
			void Remove( ObjectHandle obj );

			void GetNearby( ObjectHandle obj, std::vector< ObjectHandle >& out ) const;
			void GetNearby( const sf::Vector2f& position, std::vector< ObjectHandle >& out ) const;
			void GetNearby( ObjectHandle obj, const sf::Vector2f& topLeft, const sf::Vector2f& botRight, std::vector< ObjectHandle >& out ) const;

			template< typename Func >
			void ForEachNearby( ObjectHandle obj, Func f ) const;

			template< typename Func >
			void ForEachNearby( const sf::Vector2f& position, Func f ) const;

			template< typename Func >
			void ForEachNearby( ObjectHandle obj, const sf::Vector2f& topLeft, const sf::Vector2f& botRight, Func f ) const;

			void Reset( const bool shouldRePopulate = false );
			void Reset( const unsigned spacialHashMapGridSize, const bool shouldRePopulate = false );

		protected:
			void RemoveByID( ObjectHandle obj, const unsigned id );
			unsigned GetID( const ObjectHandle obj ) const;
			unsigned GetID( const sf::Vector2f& position ) const;
			std::vector< unsigned > GetID( const sf::Vector2f& topLeft, const sf::Vector2f& botRight ) const;
			sf::Vector2i Hash( const sf::Vector2f& position ) const;

		private:
			const sf::FloatRect m_worldBounds;
			unsigned m_storageSize = 0U;
			unsigned m_tileMapGridSize = 0U;
			unsigned m_spacialHashMapWidth = 0U;
			unsigned m_spacialHashMapHeight = 0U;
			std::vector< std::unordered_set< ObjectHandle > > m_spacialHashMap;
		};

		// Template function definitions
		template< typename Func >
		void TileMap::ForEachNearby( ObjectHandle obj, Func f ) const
		{
			if( !obj )
				return;

			const auto id = GetID( obj );
			auto& bucket = m_spacialHashMap[id];

			for( auto& item : bucket )
				if( item != obj )
					f( item );
		}

		template< typename Func >
		void TileMap::ForEachNearby( const sf::Vector2f& position, Func f ) const
		{
			const auto id = GetID( position );
			auto& bucket = m_spacialHashMap[id];

			for( auto& item : bucket )
				f( item );
		}

		template< typename Func >
		void TileMap::ForEachNearby( ObjectHandle obj, const sf::Vector2f& topLeft, const sf::Vector2f& botRight, Func f ) const
		{
			if( !obj )
				return;

			const auto ids = GetID( topLeft, botRight );

			for( auto& id : ids )
			{
				auto& bucket = m_spacialHashMap[id];

				for( auto& item : bucket )
					if( item != obj )
						f( item );
			}
		}
	}
}