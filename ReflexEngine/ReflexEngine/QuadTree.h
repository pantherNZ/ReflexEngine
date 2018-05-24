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
			QuadTree( const sf::FloatRect& boundary );

			void SetBoundary( const sf::FloatRect& boundary );
			void Subdivide();
			void Insert( const ObjectHandle& obj, const sf::FloatRect& objectBounds );

			void Query( const sf::Vector2f& position, std::vector< ObjectHandle >& out ) const;
			void Query( const sf::FloatRect& bounds, std::vector< ObjectHandle >& out ) const;
			void Query( const sf::Vector2f& position, std::vector< std::pair< ObjectHandle, sf::FloatRect > >& out ) const;
			void Query( const sf::FloatRect& bounds, std::vector< std::pair< ObjectHandle, sf::FloatRect > >& out ) const;

			void Remove( const ObjectHandle& obj, const sf::Vector2f& position );
			void Remove( const ObjectHandle& obj, const sf::FloatRect& objectBounds );

		protected:
			std::array< std::unique_ptr< QuadTree >, QUAD_TREE_CHILDREN > m_children = { nullptr, nullptr, nullptr, nullptr };
			std::vector< std::pair< ObjectHandle, sf::FloatRect > > m_objects;
			sf::FloatRect m_boundary;
		};
	}
}