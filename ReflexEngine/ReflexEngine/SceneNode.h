#pragma once

#include "Precompiled.h"
#include "HandleFwd.hpp"
#include "GRIDComponent.h"

namespace Reflex
{
	namespace Core
	{
		class SceneNode : public sf::Transformable
		{
		public:
			friend class Reflex::Components::Grid;

			SceneNode( const ObjectHandle& self );

			void AttachChild( const ObjectHandle& child );
			ObjectHandle DetachChild( const ObjectHandle& node );

			sf::Transform GetWorldTransform() const;
			sf::Vector2f GetWorldPosition() const; 

			template< typename Func >
			void ForEachChild( Func function )
			{
				std::for_each( m_children.begin(), m_children.end(), function );
			}

			unsigned GetChildrenCount() const;
			ObjectHandle GetChild( const unsigned index ) const;
			void SetZOrder( const unsigned renderIndex );
			void SetLayer( const unsigned layerIndex );

		protected:
			ObjectHandle m_owningObject;
			ObjectHandle m_parent;
			Reflex::VectorSet< ObjectHandle > m_children;
			unsigned m_renderIndex = 0U;
			unsigned m_layerIndex = 0U;

			static unsigned s_nextRenderIndex;
		};
	}
}