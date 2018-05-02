#pragma once

#include "Precompiled.h"
#include "HandleFwd.hpp"

namespace Reflex
{
	namespace Core
	{
		template< typename T, typename P = T >
		class SceneNode : public sf::Transformable
		{
		public:
			void AttachChild( const Handle< T >& child );
			bool DetachChild( const Handle< T >& node );

			sf::Transform GetWorldTransform() const;
			sf::Vector2f GetWorldPosition() const;

			template< typename Func >
			void ForEachChild( Func function )
			{
				std::for_each( m_children.begin(), m_children.end(), function );
			}

			unsigned GetChildrenCount() const;
			Handle< T > GetChild( const unsigned index ) const;
			void SetZOrder( const unsigned renderIndex );
			void SetLayer( const unsigned layerIndex );

		private:
			virtual Handle< P > GetHandle() const = 0;

		private:
			std::vector< Handle< T > > m_children;
			unsigned m_renderIndex = 0U;
			unsigned m_layerIndex = 0U;

			static unsigned m_nextRenderIndex;

		public:
			Handle< P > m_parent;
		};

		template< typename T, typename P >
		unsigned SceneNode< T, P >::m_nextRenderIndex = 0U;

		// Template definitions
		template< typename T, typename P >
		void SceneNode< T, P >::AttachChild( const Handle< T >& child )
		{
			const auto test = child.Get();
			if( child->m_parent )
				child->m_parent->DetachChild( child );

			child->m_parent = GetHandle();
			m_children.push_back( std::move( child ) );
			child->SetZOrder( m_nextRenderIndex++ );
			child->SetLayer( m_layerIndex + 1 );
		}

		template< typename T, typename P = T >
		bool SceneNode< T, P >::DetachChild( const Handle< T >& node )
		{
			// Find and detach the node
			const auto found = std::find( m_children.begin(), m_children.end(), node );

			if( found == m_children.end() )
			{
				LOG_CRIT( "SceneNode::DetachChild | Node not found" );
				return false;
			}

			auto found_node = std::move( *found );
			found_node->m_parent = Handle< T >::null;
			m_children.erase( found );
			return true;
		}

		template< typename T, typename P = T >
		sf::Transform SceneNode< T, P >::GetWorldTransform() const
		{
			sf::Transform worldTransform;

			for( Handle< T > node = GetHandle(); node != Handle< T >::null; node = node->m_parent )
				worldTransform = node->getTransform() * worldTransform;

			return worldTransform;
		}

		template< typename T, typename P = T >
		sf::Vector2f SceneNode< T, P >::GetWorldPosition() const
		{
			return GetWorldTransform() * sf::Vector2f();
		}

		template< typename T, typename P = T >
		unsigned SceneNode< T, P >::GetChildrenCount() const
		{
			return m_children.size();
		}

		template< typename T, typename P = T >
		Handle< T > SceneNode< T, P >::GetChild( const unsigned index ) const
		{
			if( index >= GetChildrenCount() )
			{
				LOG_CRIT( "SceneNode::GetChild | Tried to get a child with an invalid index: " + index );
				return Handle< T >::null;
			}

			return m_children[index];
		}

		template< typename T, typename P = T >
		void SceneNode< T, P >::SetZOrder( const unsigned renderIndex )
		{
			m_renderIndex = renderIndex;
		}

		template< typename T, typename P = T >
		void SceneNode< T, P >::SetLayer( const unsigned layerIndex )
		{
			m_layerIndex = layerIndex;
		}
	}
}