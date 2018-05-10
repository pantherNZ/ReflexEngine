#include "QuadTree.h"

namespace Reflex
{
	namespace Core
	{
		QuadTree::QuadTree( const AABB& boundary )
			: m_boundary( boundary )
		{
			m_objects.reserve( QUAD_TREE_SPACE );
		}

		void QuadTree::SetBoundary( const AABB& boundary )
		{
			m_boundary = boundary;
		}

		void QuadTree::Subdivide()
		{
			const auto newHalfSize = m_boundary.halfSize / 2.0f;
			m_children[0] = std::make_unique< QuadTree >( AABB( m_boundary.centre + sf::Vector2f( -newHalfSize.x, -newHalfSize.y ), newHalfSize ) );
			m_children[1] = std::make_unique< QuadTree >( AABB( m_boundary.centre + sf::Vector2f( newHalfSize.x, -newHalfSize.y ), newHalfSize ) );
			m_children[2] = std::make_unique< QuadTree >( AABB( m_boundary.centre + sf::Vector2f( -newHalfSize.x, newHalfSize.y ), newHalfSize ) );
			m_children[3] = std::make_unique< QuadTree >( AABB( m_boundary.centre + sf::Vector2f( newHalfSize.x, newHalfSize.y ), newHalfSize ) );
		}

		void QuadTree::Insert( const ObjectHandle& obj, const AABB& objectBounds )
		{
			if( !m_boundary.Intersects( objectBounds ) )
				return;

			if( m_objects.size() < QUAD_TREE_SPACE )
			{
				m_objects.push_back( std::make_pair( obj, objectBounds ) );
			}
			else
			{
				if( !m_children[0] )
					Subdivide();

				for( unsigned i = 0U; i < QUAD_TREE_CHILDREN; ++i )
					m_children[i]->Insert( obj, objectBounds );
			}
		}

		void QuadTree::Query( const sf::Vector2f& position, std::vector< ObjectHandle >& out ) const
		{
			Query( AABB( position ), out );
		}

		void QuadTree::Query( const AABB& bounds, std::vector< ObjectHandle >& out ) const
		{
			if( !m_boundary.Intersects( bounds ) )
				return;

			for( auto& item : m_objects )
				if( item.second.Intersects( bounds ) )
					out.push_back( item.first );

			if( m_children[0] )
				for( unsigned i = 0U; i < QUAD_TREE_CHILDREN; ++i )
					m_children[i]->Query( bounds, out );
		}

		void QuadTree::Query( const sf::Vector2f& position, std::vector< std::pair< ObjectHandle, AABB > >& out ) const
		{
			Query( AABB( position ), out );
		}

		void QuadTree::Query( const AABB& bounds, std::vector< std::pair< ObjectHandle, AABB > >& out ) const
		{
			if( !m_boundary.Intersects( bounds ) )
				return;

			for( auto& item : m_objects )
				if( item.second.Intersects( bounds ) )
					out.push_back( item );

			if( m_children[0] )
				for( unsigned i = 0U; i < QUAD_TREE_CHILDREN; ++i )
					m_children[i]->Query( bounds, out );
		}

		ObjectHandle QuadTree::FindBottomMost( const sf::Vector2f& position ) const
		{
			return FindBottomMost( AABB( position ) );
		}

		ObjectHandle QuadTree::FindBottomMost( const AABB& bounds ) const
		{
			if( !m_boundary.Intersects( bounds ) )
				return ObjectHandle::null;

			std::vector< std::pair< ObjectHandle, AABB > > valid;

			for( auto& item : m_objects )
				if( item.second.Intersects( bounds ) )
					valid.push_back( item );

			if( m_children[0] )
				for( unsigned i = 0U; i < QUAD_TREE_CHILDREN; ++i )
					m_children[i]->Query( bounds, valid );

			if( valid.empty() )
				return ObjectHandle::null;

			unsigned bestIndex = 0U;
			float smallest = std::numeric_limits< float >::infinity();

			for( unsigned i = 0U; i < valid.size(); ++i )
			{
				if( valid[i].second.halfSize.x * valid[i].second.halfSize.y < smallest )
				{
					smallest = valid[i].second.halfSize.x * valid[i].second.halfSize.y;
					bestIndex = i;
				}
			}

			return valid[bestIndex].first;
		}

		void QuadTree::Remove( const ObjectHandle& obj, const sf::Vector2f& position )
		{
			Remove( obj, AABB( position ) );
		}

		void QuadTree::Remove( const ObjectHandle& obj, const AABB& objectBounds )
		{
			if( !m_boundary.Intersects( objectBounds ) )
				return;

			for( auto& iter = m_objects.begin(); iter != m_objects.end(); ++iter )
			{
				if( iter->first == obj )
				{
					*iter = m_objects.back();
					m_objects.pop_back();
					return;
				}
			}

			if( m_children[0] )
				for( unsigned i = 0U; i < QUAD_TREE_CHILDREN; ++i )
					m_children[i]->Remove( obj, objectBounds );
		}
	}
}