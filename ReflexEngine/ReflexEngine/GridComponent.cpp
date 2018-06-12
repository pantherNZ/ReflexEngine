#include "GridComponent.h"
#include "Object.h"
#include "World.h"

namespace Reflex
{
	namespace Components
	{
		Grid::Grid( const unsigned width, const unsigned height, const float cellWidth, const float cellHeight )
			: m_gridSize( width, height )
			, m_cellSize( cellWidth, cellHeight )
		{
			assert( width != 0U && height != 0U );
			m_children.resize( GetTotalCells() );
		}

		Grid::Grid( const sf::Vector2u gridSize, const sf::Vector2f cellSize )
			: m_gridSize( gridSize )
			, m_cellSize( cellSize )
		{
			assert( m_gridSize.x != 0U && m_gridSize.y != 0U );
			m_children.resize( GetTotalCells() );
		}

		void Grid::AddToGrid( const ObjectHandle& handle, const unsigned x, const unsigned y )
		{
			AddToGrid( handle, sf::Vector2u( x, y ) );
		}

		void Grid::AddToGrid( const ObjectHandle& handle, const sf::Vector2u index )
		{
			if( index.x >= GetWidth() || index.y >= GetHeight() )
			{
				LOG_CRIT( "Trying to add to grid with an invalid index of (" << index.x << ", " << index.y << ")" );
				return;
			}
			
			auto transform = handle->GetTransform();

			if( transform->m_parent )
				transform->m_parent->GetTransform()->DetachChild( handle );

			const auto insertIndex = GetIndex( index );
			m_children[insertIndex] = handle;
			transform->m_parent = GetObject();
			transform->SetZOrder( SceneNode::s_nextRenderIndex++ );
			transform->SetLayer( GetObject()->GetTransform()->m_layerIndex + 1 );
			transform->setPosition( GetCellPosition( index ) );
		}

		ObjectHandle Grid::RemoveFromGrid( const unsigned x, const unsigned y )
		{
			return RemoveFromGrid( sf::Vector2u( x, y ) );
		}

		ObjectHandle Grid::RemoveFromGrid( const sf::Vector2u index )
		{
			if( index.x >= GetWidth() || index.y >= GetHeight() )
			{
				LOG_CRIT( "Trying to remove from grid with an invalid index of (" << index.x << ", " << index.y << ")" );
				return ObjectHandle::null;
			}

			const auto insertIndex = GetIndex( index );
			const auto result = m_children[insertIndex];
			GetObject()->GetWorld().m_sceneGraphRoot->AttachChild( result );
			m_children[insertIndex] = ObjectHandle::null;
			return result;
		}

		ObjectHandle Grid::GetCell( const unsigned x, const unsigned y ) const
		{
			return GetCell( sf::Vector2u( x, y ) );
		}

		ObjectHandle Grid::GetCell( const sf::Vector2u index ) const
		{
			if( index.x >= GetWidth() || index.y >= GetHeight() )
			{
				LOG_CRIT( "Trying to access from grid with an invalid index of (" << index.x << ", " << index.y << ")" );
				return TransformHandle::null;
			}

			const auto insertIndex = GetIndex( index );
			return m_children[insertIndex];
		}

		unsigned Grid::GetWidth() const
		{
			return m_gridSize.x;
		}

		unsigned Grid::GetHeight() const
		{
			return m_gridSize.y;
		}

		sf::Vector2u Grid::GetGridSize() const
		{
			return m_gridSize;
		}

		unsigned Grid::GetTotalCells() const
		{
			return m_gridSize.x * m_gridSize.y;
		}

		sf::Vector2f Grid::GetCellSize() const
		{
			return m_cellSize;
		}

		void Grid::SetCellSize( const sf::Vector2f cellSize )
		{
			if( m_cellSize != cellSize )
			{
				m_cellSize = cellSize;
				UpdateGridPositions();
			}
		}

		void Grid::SetGridIsCentred( const bool centreGrid )
		{
			if( centreGrid != m_centreGrid )
			{
				m_centreGrid = centreGrid;
				UpdateGridPositions();
			}
		}

		bool Grid::GetGridIsCentred() const
		{
			return m_centreGrid;
		}

		sf::Vector2f Grid::GetCellPosition( const sf::Vector2u index ) const
		{
			const auto topLeft = m_centreGrid ? sf::Vector2f( ( GetWidth() / -2.0f + 0.5f ) * m_cellSize.x, ( GetHeight() / -2.0f + 0.5f ) * m_cellSize.y ) : sf::Vector2f( 0.0f, 0.0f );
			const auto position = topLeft + sf::Vector2f( index.x * m_cellSize.x, index.y * m_cellSize.y ); //+ gridCentre
			const auto transform = GetObject()->GetTransform();
			const auto gridCentre = transform->GetWorldPosition();
			return Reflex::RotateAroundPoint( position, gridCentre, transform->GetWorldRotation() );
		}

		std::pair< bool, sf::Vector2u > Grid::GetCellIndex( const sf::Vector2f position ) const
		{
			const auto transform = GetObject()->GetTransform();
			const auto gridCentre = transform->GetWorldPosition();
			const auto localPosition = position - ( gridCentre + GetCellPosition( sf::Vector2u( 0U, 0U ) ) );
			const auto rotatedPosition = Reflex::RotateAroundPoint( localPosition, gridCentre, -transform->GetWorldRotation() );
			const auto indexX = RoundToInt( rotatedPosition.x / m_cellSize.x );
			const auto indexY = RoundToInt( rotatedPosition.y / m_cellSize.y );
			return std::make_pair( indexX >= 0 && indexX < ( int )GetWidth() && indexY >= 0 && indexY < ( int )GetHeight(), sf::Vector2u( indexX, indexY ) );
		}

		void Grid::ForEachChild( std::function< void( const ObjectHandle& obj, const sf::Vector2u index ) > callback )
		{
			for( unsigned i = 0U; i < m_children.size(); ++i )
				callback( m_children[i], GetCoords( i ) );
		}

		unsigned Grid::GetIndex( const sf::Vector2u coords ) const
		{
			return coords.y * GetWidth() + coords.x;
		}

		const sf::Vector2u Grid::GetCoords( const unsigned index ) const
		{
			return sf::Vector2u( index % GetWidth(), index / GetWidth() );
		}

		void Grid::UpdateGridPositions()
		{
			for( unsigned y = 0U; y < GetHeight(); ++y )
			{
				for( unsigned x = 0U; x < GetWidth(); ++x )
				{
					const auto index = sf::Vector2u( x, y );
					m_children[GetIndex( index )]->GetTransform()->setPosition( GetCellPosition( index ) );
				}
			}
		}
	}
}