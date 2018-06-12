#include "SceneNode.h"
#include "Object.h"
#include "TransformComponent.h"

namespace Reflex
{
	namespace Core
	{
		unsigned SceneNode::s_nextRenderIndex = 0U;

		void SceneNode::AttachChild( const ObjectHandle& child )
		{
			auto transform = child->GetTransform();

			if( transform->m_parent )
				transform->m_parent->GetTransform()->DetachChild( child );

			transform->m_parent = m_owningObject;
			transform->SetZOrder( s_nextRenderIndex++ );
			transform->SetLayer( m_layerIndex + 1 );
			//m_children.insert( child );
			m_children.push_back( child );
		}

		ObjectHandle SceneNode::DetachChild( const ObjectHandle& node )
		{
			// Find and detach the node'
			for( unsigned i = 0U; i < m_children.size(); ++i )
			{
				if( node == m_children[i] )
				{
					m_children[i]->GetTransform()->m_parent = ObjectHandle::null;
					m_children.erase( m_children.begin() + i );
					return node;
				}
			}

			LOG_CRIT( "Node not found" );
			return ObjectHandle::null;
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

		sf::Vector2f SceneNode::GetWorldTranslation() const
		{
			sf::Vector2f worldTranslation;

			for( ObjectHandle node = m_owningObject; node != ObjectHandle::null; node = node->GetTransform()->m_parent )
			{
				worldTranslation.x += node->GetTransform()->getPosition().x;
				worldTranslation.y += node->GetTransform()->getPosition().y;
			}

			return worldTranslation;
		}

		float SceneNode::GetWorldRotation() const
		{
			float worldRotation = 0.0f;

			for( ObjectHandle node = m_owningObject; node != ObjectHandle::null; node = node->GetTransform()->m_parent )
				worldRotation += node->GetTransform()->getRotation();

			return worldRotation;
		}

		sf::Vector2f SceneNode::GetWorldScale() const
		{
			sf::Vector2f worldScale( 1.0f, 1.0f );

			for( ObjectHandle node = m_owningObject; node != ObjectHandle::null; node = node->GetTransform()->m_parent )
			{
				worldScale.x *= node->GetTransform()->getScale().x;
				worldScale.y *= node->GetTransform()->getScale().y;
			}

			return worldScale;
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

		ObjectHandle SceneNode::GetParent() const
		{
			return m_parent;
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