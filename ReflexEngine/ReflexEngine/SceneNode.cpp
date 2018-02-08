
#include "SceneNode.h"

namespace Reflex
{
	namespace Core
	{
		SceneNode::SceneNode()
		{

		}

		void SceneNode::AttachChild( std::unique_ptr< SceneNode > child )
		{
			child->m_parent = this;
			m_children.push_back( std::move( child ) );
		}

		std::unique_ptr< SceneNode > SceneNode::DetachChild( const SceneNode& node )
		{
			// Find and detach the node
			const auto found = std::find_if( m_children.begin(), m_children.end(), [&node]( const std::unique_ptr< SceneNode >& child ) { return &node == child.get(); } );

			if( found == m_children.end() )
			{
				LOG_CRIT( "WorldNode::DetachChild | Node not found" );
				return nullptr;
			}

			auto found_node = std::move( *found );
			found_node->m_parent = nullptr;
			m_children.erase( found );
			return found_node;
		}

		void SceneNode::Update( const sf::Time deltaTime )
		{
			UpdateCurrent( deltaTime );

			for( auto& child : m_children )
				child->Update( deltaTime );
		}

		sf::Transform SceneNode::GetWorldTransform() const
		{
			sf::Transform worldTransform;

			for( const SceneNode* node = this; node != nullptr; node = node->m_parent )
				worldTransform = node->getTransform() * worldTransform;

			return worldTransform;
		}

		sf::Vector2f SceneNode::GetWorldPosition() const
		{
			return GetWorldTransform() * sf::Vector2f();
		}

		unsigned SceneNode::GetChildrenCount() const
		{
			return m_children.size();
		}
		
		SceneNode* SceneNode::GetChild( const unsigned index )
		{
			if( index >= GetChildrenCount())
			{
				LOG_CRIT( "SceneNode::GetChild | Tried to get a child with an invalid index: " + index );
				return nullptr;
			}

			return m_children[index].get();
		}

		void SceneNode::draw( sf::RenderTarget& target, sf::RenderStates states ) const
		{
			states.transform *= getTransform();

			DrawCurrent( target, states );

			for( auto& child : m_children )
				child->draw( target, states );
		}
	}
}