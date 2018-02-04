
#include "WorldNode.h"

namespace Reflex
{
	namespace Core
	{
		WorldNode::WorldNode()
		{

		}

		void WorldNode::AttachChild( std::unique_ptr< WorldNode > child )
		{
			child->m_parent = this;
			m_children.push_back( std::move( child ) );
		}

		std::unique_ptr< WorldNode > WorldNode::DetachChild( const WorldNode& node )
		{
			// Find and detach the node
			const auto found = std::find_if( m_children.begin(), m_children.end(), [&node]( const std::unique_ptr< WorldNode >& child ) { return &node == child.get(); } );

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

		void WorldNode::Update( const sf::Time deltaTime )
		{
			UpdateCurrent( deltaTime );

			for( auto& child : m_children )
				child->Update( deltaTime );
		}

		sf::Transform WorldNode::GetWorldTransform() const
		{
			sf::Transform worldTransform;

			for( const WorldNode* node = this; node != nullptr; node = node->m_parent )
				worldTransform = node->getTransform() * worldTransform;

			return worldTransform;
		}

		sf::Vector2f WorldNode::GetWorldPosition() const
		{
			return GetWorldTransform() * sf::Vector2f();
		}

		void WorldNode::draw( sf::RenderTarget& target, sf::RenderStates states ) const
		{
			states.transform *= getTransform();

			DrawCurrent( target, states );

			for( auto& child : m_children )
				child->draw( target, states );
		}
	}
}