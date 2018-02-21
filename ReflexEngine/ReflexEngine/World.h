#pragma once

#include "Common.h"
#include "ResourceManager.h"
#include "Object.h"
#include "ObjectAllocator.h"

// Engine class
namespace Reflex
{
	namespace Core
	{
		// World class
		class World : private sf::NonCopyable
		{
		public:
			explicit World( sf::RenderTarget& window );

			void Update( const sf::Time deltaTime );
			void Render();

			Object& CreateObject();
			void DestroyObject( Object& object );

		protected:
			//void BuildScene();

		private:
			// Remove
			World() = delete;
			World( const ObjectAllocator& ) = delete;
			World& operator=( const ObjectAllocator& ) = delete;

		protected:
			enum Variables
			{
				MaxLayers = 5,
			};

			sf::RenderTarget& mWindow;
			sf::View mWorldView;

			ObjectAllocator mObjects;
			std::vector< std::unique_ptr< ObjectAllocator> > mComponents;
			std::vector< Object& > mMarkedForDeletion;
		};
	}
}