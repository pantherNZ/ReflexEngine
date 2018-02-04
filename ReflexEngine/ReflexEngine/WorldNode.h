#pragma once

#include "Common.h"

namespace Reflex
{
	namespace Core
	{
		class WorldNode : public sf::Transformable, public sf::Drawable, private sf::NonCopyable
		{
		public:
			WorldNode();

			void AttachChild( std::unique_ptr< WorldNode > child );
			std::unique_ptr< WorldNode > DetachChild( const WorldNode& node );
			void Update( const sf::Time deltaTime );

			sf::Transform GetWorldTransform() const;
			sf::Vector2f GetWorldPosition() const;

		private:
			virtual void UpdateCurrent( const sf::Time deltaTime ) { }
			void draw( sf::RenderTarget& target, sf::RenderStates states ) const final;
			virtual void DrawCurrent( sf::RenderTarget& target, sf::RenderStates states ) const { }

		private:
			std::vector< std::unique_ptr< WorldNode > > m_children;
			WorldNode* m_parent = nullptr;
		};
	}
}