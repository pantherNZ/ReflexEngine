#include "QuadTree.h"

namespace Reflex
{
	namespace Core
	{
		QuadTree::QuadTree( const sf::FloatRect& boundary )
			: m_boundary( boundary )
		{
			m_objects.reserve( QUAD_TREE_SPACE );
		}

		void QuadTree::SetBoundary( const sf::FloatRect& boundary )
		{
			m_boundary = boundary;
		}

		void QuadTree::Subdivide()
		{
			const auto centre = sf::Vector2f( m_boundary.left + m_boundary.width / 2.0f, m_boundary.top + m_boundary.height / 2.0f );
			const auto newHalfSize = sf::Vector2f( m_boundary.width / 2.0f, m_boundary.height / 2.0f );
			m_children[0] = std::make_unique< QuadTree >( sf::FloatRect( centre + sf::Vector2f( -newHalfSize.x, -newHalfSize.y ), newHalfSize ) );
			m_children[1] = std::make_unique< QuadTree >( sf::FloatRect( centre + sf::Vector2f( newHalfSize.x, -newHalfSize.y ), newHalfSize ) );
			m_children[2] = std::make_unique< QuadTree >( sf::FloatRect( centre + sf::Vector2f( -newHalfSize.x, newHalfSize.y ), newHalfSize ) );
			m_children[3] = std::make_unique< QuadTree >( sf::FloatRect( centre + sf::Vector2f( newHalfSize.x, newHalfSize.y ), newHalfSize ) );
		}

		void QuadTree::Insert( const ObjectHandle& obj, const sf::FloatRect& objectBounds )
		{
			if( !m_boundary.intersects( objectBounds ) )
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
			Query( sf::FloatRect( position, sf::Vector2f( 0.0f, 0.0f ) ), out );
		}

		void QuadTree::Query( const sf::FloatRect& bounds, std::vector< ObjectHandle >& out ) const
		{
			if( !m_boundary.intersects( bounds ) )
				return;

			for( auto& item : m_objects )
				if( item.second.intersects( bounds ) )
					out.push_back( item.first );

			if( m_children[0] )
				for( unsigned i = 0U; i < QUAD_TREE_CHILDREN; ++i )
					m_children[i]->Query( bounds, out );
		}

		void QuadTree::Query( const sf::Vector2f& position, std::vector< std::pair< ObjectHandle, sf::FloatRect > >& out ) const
		{
			Query( sf::FloatRect( position, sf::Vector2f( 0.0f, 0.0f ) ), out );
		}

		void QuadTree::Query( const sf::FloatRect& bounds, std::vector< std::pair< ObjectHandle, sf::FloatRect > >& out ) const
		{
			if( !m_boundary.intersects( bounds ) )
				return;

			for( auto& item : m_objects )
				if( item.second.intersects( bounds ) )
					out.push_back( item );

			if( m_children[0] )
				for( unsigned i = 0U; i < QUAD_TREE_CHILDREN; ++i )
					m_children[i]->Query( bounds, out );
		}

		void QuadTree::Remove( const ObjectHandle& obj, const sf::Vector2f& position )
		{
			Remove( obj, sf::FloatRect( position, sf::Vector2f( 0.0f, 0.0f ) ) );
		}

		void QuadTree::Remove( const ObjectHandle& obj, const sf::FloatRect& objectBounds )
		{
			if( !m_boundary.intersects( objectBounds ) )
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