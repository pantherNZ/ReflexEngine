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

			void AttachChild( const ObjectHandle& child );
			ObjectHandle DetachChild( const ObjectHandle& node );

			sf::Transform GetWorldTransform() const;
			sf::Vector2f GetWorldPosition() const; 

			sf::Vector2f GetWorldTranslation() const;
			float GetWorldRotation() const;
			sf::Vector2f GetWorldScale() const;

			template< typename Func >
			void ForEachChild( Func function )
			{
				std::for_each( m_children.begin(), m_children.end(), function );
			}

			unsigned GetChildrenCount() const;
			ObjectHandle GetChild( const unsigned index ) const;
			ObjectHandle GetParent() const;
			void SetZOrder( const unsigned renderIndex );
			unsigned GetZOrder() const;
			void SetLayer( const unsigned layerIndex );
			unsigned GetRenderIndex() const;

		protected:
			ObjectHandle m_owningObject;
			ObjectHandle m_parent;
		//	Reflex::VectorSet< ObjectHandle > m_children;
			std::vector< ObjectHandle > m_children;
			unsigned m_renderIndex = 0U;
			unsigned m_layerIndex = 0U;

			static unsigned s_nextRenderIndex;
		};
	}
}