#include "QuadTree.h"

namespace Reflex
{
	namespace Core
	{
		QuadTree::QuadTree( AABB boundary ) 
			: boundary( boundary ) 
		{ 
			objects.reserve( QUAD_TREE_SPACE ); 
		}

		void QuadTree::Subdivide()
		{
			const auto newHalfSize = boundary.halfSize / 2.0f;
			children[0] = std::make_unique< QuadTree >( AABB( boundary.centre + sf::Vector2f( -newHalfSize.x, -newHalfSize.y ), newHalfSize ) );
			children[1] = std::make_unique< QuadTree >( AABB( boundary.centre + sf::Vector2f( newHalfSize.x, -newHalfSize.y ), newHalfSize ) );
			children[2] = std::make_unique< QuadTree >( AABB( boundary.centre + sf::Vector2f( -newHalfSize.x, newHalfSize.y ), newHalfSize ) );
			children[3] = std::make_unique< QuadTree >( AABB( boundary.centre + sf::Vector2f( newHalfSize.x, newHalfSize.y ), newHalfSize ) );
		}

		void QuadTree::Insert( const ObjectHandle& obj, const AABB& objectBounds )
		{
			if( !boundary.Intersects( objectBounds ) )
				return;

			if( objects.size() < QUAD_TREE_SPACE )
			{
				objects.push_back( std::make_pair( obj, objectBounds ) );
			}
			else
			{
				if( !children[0] )
					Subdivide();

				for( unsigned i = 0U; i < QUAD_TREE_CHILDREN; ++i )
					children[i]->Insert( obj, objectBounds );
			}
		}

		void QuadTree::Query( const AABB& bounds, std::vector< ObjectHandle >& out ) const
		{
			if( !boundary.Intersects( bounds ) )
				return;

			for( auto& item : objects )
				if( item.second.Intersects( bounds ) )
					out.push_back( item.first );

			if( children[0] )
				for( unsigned i = 0U; i < QUAD_TREE_CHILDREN; ++i )
					children[i]->Query( bounds, out );
		}

		void QuadTree::Remove( const ObjectHandle& obj, const AABB& objectBounds )
		{
			if( !boundary.Intersects( objectBounds ) )
				return;

			for( auto& iter = objects.begin(); iter != objects.end(); ++iter )
			{
				if( iter->first == obj )
				{
					*iter = objects.back();
					objects.pop_back();
					return;
				}
			}

			if( children[0] )
				for( unsigned i = 0U; i < QUAD_TREE_CHILDREN; ++i )
					children[i]->Remove( obj, objectBounds );
		}
	}
}