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
			QuadTree( AABB boundary );

			void Subdivide();
			void Insert( const ObjectHandle& obj, const AABB& objectBounds );
			void Query( const AABB& bounds, std::vector< ObjectHandle >& out ) const;
			void Remove( const ObjectHandle& obj, const AABB& objectBounds );

		protected:
			std::array< std::unique_ptr< QuadTree >, QUAD_TREE_CHILDREN > children = { nullptr, nullptr, nullptr, nullptr };
			std::vector< std::pair< ObjectHandle, AABB > > objects;
			AABB boundary;
		};
	}
}