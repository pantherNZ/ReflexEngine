#include "SceneNode.h"
#include "Object.h"
#include "TransformComponent.h"

namespace Reflex
{
	namespace Core
	{
		unsigned SceneNode::s_nextRenderIndex = 0U;

		SceneNode::SceneNode( const ObjectHandle& self )
			: m_owningObject( self )
		{

		}

		void SceneNode::AttachChild( const ObjectHandle& child )
		{
			auto transform = child->GetTransform();

			if( transform->m_parent )
				transform->m_parent->GetTransform()->DetachChild( child );

			transform->m_parent = m_owningObject;
			transform->SetZOrder( s_nextRenderIndex++ );
			transform->SetLayer( m_layerIndex + 1 );
			m_children.insert( child );
		}

		ObjectHandle SceneNode::DetachChild( const ObjectHandle& node )
		{
			// Find and detach the node
			const auto found = std::find( m_children.begin(), m_children.end(), node );

			if( found == m_children.end() )
			{
				LOG_CRIT( "Node not found" );
				return ObjectHandle::null;
			}

			auto found_node = std::move( *found );
			found_node->GetTransform()->m_parent = ObjectHandle::null;
			m_children.erase( found );
			return found_node;
		}

		sf::Transform SceneNode::GetWorldTransform() const
		{
			TODO( "Cache the world transform" );
			sf::Transform worldTransform;

			for( ObjectHandle node = m_owningObject; node != ObjectHandle::null; node = node->GetTransform()->m_parent )
				worldTransform = node->GetTransform()->getTransform() * worldTransform;

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

		ObjectHandle SceneNode::GetChild( const unsigned index ) const
		{
			if( index >= GetChildrenCount() )
			{
				LOG_CRIT( "Tried to get a child with an invalid index: " + index );
				return ObjectHandle::null;
			}

			return m_children[index];
		}

		void SceneNode::SetZOrder( const unsigned renderIndex )
		{
			m_renderIndex = renderIndex;
		}

		void SceneNode::SetLayer( const unsigned layerIndex )
		{
			m_layerIndex = layerIndex;
		}
	}
}