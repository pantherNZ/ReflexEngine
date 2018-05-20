#pragma once

#include "Component.h"

namespace Reflex
{
	namespace Components
	{
		class Grid;
	}

	namespace Core
	{
		typedef Handle< class Reflex::Components::Grid > GridHandle;
	}

	namespace Components
	{
		// Class definition
		class Grid : public Component
		{
		public:
			Grid( const unsigned width, const unsigned height, const float cellWidth, const float cellHeight );
			Grid( const sf::Vector2u gridSize, const sf::Vector2f cellSize );

			void AddToGrid( const ObjectHandle& handle, const unsigned x, const unsigned y );
			void AddToGrid( const ObjectHandle& handle, const sf::Vector2u index );
			ObjectHandle RemoveFromGrid( const unsigned x, const unsigned y );
			ObjectHandle RemoveFromGrid( const sf::Vector2u index );
			ObjectHandle GetCell( const unsigned x, const unsigned y ) const;
			ObjectHandle GetCell( const sf::Vector2u index ) const;

			unsigned GetWidth() const;
			unsigned GetHeight() const;
			sf::Vector2u GetGridSize() const;
			unsigned GetTotalCells() const;

			sf::Vector2f GetCellSize() const;
			void SetCellSize( const sf::Vector2f cellSize );

			void SetGridIsCentred( const bool centreGrid );
			bool GetGridIsCentred() const;

			sf::Vector2f GetCellPosition( const sf::Vector2u index ) const;
			std::pair< bool, sf::Vector2u > GetCellIndex( const sf::Vector2f position ) const;

			template< typename Func >
			void ForEachCell( Func function )
			{
				Reflex::Core::SceneNode< Transform, Grid >::ForEachChild( function );
			}

		protected:
			unsigned GetIndex( const sf::Vector2u index ) const;
			void UpdateGridPositions();

		private:
			Reflex::VectorSet< ObjectHandle > m_children;
			sf::Vector2u m_gridSize;
			sf::Vector2f m_cellSize;
			bool m_centreGrid = true;
		};
	}
}