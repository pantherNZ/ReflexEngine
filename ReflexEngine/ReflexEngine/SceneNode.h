#pragma once

#include "Precompiled.h"

namespace Reflex
{
	namespace Core
	{
		class SceneNode : public sf::Transformable, public sf::Drawable, private sf::NonCopyable
		{
		public:
			SceneNode();

			void AttachChild( std::unique_ptr< SceneNode > child );
			std::unique_ptr< SceneNode > DetachChild( const SceneNode& node );
			void Update( const float deltaTime );

			sf::Transform GetWorldTransform() const;
			sf::Vector2f GetWorldPosition() const;

			template< typename Func >
			void ForEachChild( Func function )
			{
				std::for_each( m_children.begin(), m_children.end(), function );
			}

			unsigned GetChildrenCount() const;
			SceneNode* GetChild( const unsigned index );

		private:
			virtual void UpdateCurrent( const float deltaTime ) { }
			void draw( sf::RenderTarget& target, sf::RenderStates states ) const final;
			virtual void DrawCurrent( sf::RenderTarget& target, sf::RenderStates states ) const { }

		private:
			std::vector< std::unique_ptr< SceneNode > > m_children;
			SceneNode* m_parent = nullptr;
		};
	}
}