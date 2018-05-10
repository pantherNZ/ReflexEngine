#pragma once

#include "Utility.h"
#include "HandleFwd.hpp"
#include "Precompiled.h"

#include <array>

// Quad tree implementation
namespace Reflex
{
	namespace Core
	{
		class QuadTree
		{
			#define QUAD_TREE_CHILDREN 4U
			#define QUAD_TREE_SPACE 4U
		public:
			QuadTree( const AABB& boundary );

			void SetBoundary( const AABB& boundary );
			void Subdivide();
			void Insert( const ObjectHandle& obj, const AABB& objectBounds );

			void Query( const sf::Vector2f& position, std::vector< ObjectHandle >& out ) const;
			void Query( const AABB& bounds, std::vector< ObjectHandle >& out ) const;
			void Query( const sf::Vector2f& position, std::vector< std::pair< ObjectHandle, AABB > >& out ) const;
			void Query( const AABB& bounds, std::vector< std::pair< ObjectHandle, AABB > >& out ) const;

			ObjectHandle FindBottomMost( const sf::Vector2f& position ) const;
			ObjectHandle FindBottomMost( const AABB& bounds ) const;

			void Remove( const ObjectHandle& obj, const sf::Vector2f& position );
			void Remove( const ObjectHandle& obj, const AABB& objectBounds );

		protected:
			std::array< std::unique_ptr< QuadTree >, QUAD_TREE_CHILDREN > m_children = { nullptr, nullptr, nullptr, nullptr };
			std::vector< std::pair< ObjectHandle, AABB > > m_objects;
			AABB m_boundary;
		};
	}
}