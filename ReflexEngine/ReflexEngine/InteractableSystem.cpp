#include "World.h"
#include "InteractableSystem.h"
#include "InteractableComponent.h"
#include "SFMLObjectComponent.h"

using namespace Reflex::Components;

namespace Reflex
{
	namespace Systems
	{
		void InteractableSystem::RegisterComponents()
		{
			RequiresComponent( Transform );
			RequiresComponent( Interactable );
			RequiresComponent( SFMLObject );
		}

		bool InteractableSystem::CheckCollision( const TransformHandle& transform, const sf::FloatRect& localBounds, const sf::Vector2f& mousePosition ) const
		{
			sf::Transform transformFinal;
			transformFinal.scale( transform->GetWorldScale() ).translate( transform->GetWorldTranslation() );
			auto globalBounds = transformFinal.transformRect( localBounds );
			globalBounds.left -= localBounds.width / 2.0f;
			globalBounds.top -= localBounds.height / 2.0f;
			return Reflex::BoundingBox( globalBounds, transform->GetWorldRotation() ).contains( mousePosition );
		}

		void InteractableSystem::Update( const float deltaTime )
		{
			const auto mousePosition = Reflex::ToVector2f( sf::Mouse::getPosition( *GetWorld().GetContext().window ) );

			ForEachSystemComponent< Transform, Interactable, SFMLObject >(
				[&]( const TransformHandle& transform, InteractableHandle& interactable, const SFMLObjectHandle& sfmlObj )
			{
				bool collision = false;
				
				// Collision with bounds
				switch( sfmlObj->GetType() )
				{
				case SFMLObjectType::Circle:
					collision = Reflex::Circle( transform->GetWorldPosition(), sfmlObj->GetCircleShape().getRadius() ).Contains( mousePosition );
				break;
				case SFMLObjectType::Rectangle:
					collision = CheckCollision( transform, sfmlObj->GetRectangleShape().getLocalBounds(), mousePosition );
				break;
				case SFMLObjectType::Convex:
					collision = CheckCollision( transform, sfmlObj->GetConvexShape().getLocalBounds(), mousePosition );
				break;
				case SFMLObjectType::Sprite:
					collision = CheckCollision( transform, sfmlObj->GetSprite().getLocalBounds(), mousePosition );
				break;
				}

				auto* ptr = interactable.Get();

				// Focus / highlighting
				if( ptr->m_isFocussed != collision )
				{
					ptr->m_isFocussed = collision;

					if( !collision && ptr->m_lostFocusCallback )
						ptr->m_lostFocusCallback( interactable );
					else if( collision && ptr->m_gainedFocusCallback )
						ptr->m_gainedFocusCallback( interactable );

					// Lost highlight, then we also unselect
					if( !collision && !ptr->m_selectionIsToggle && ptr->m_unselectIfLostFocus )
						ptr->Deselect();
				}

				// Selection (or can be deselection for toggle mode)
				if( ptr->m_isFocussed && m_mousePressed )
				{
					ptr->m_isSelected && ptr->m_selectionIsToggle ? ptr->Deselect() : ptr->Select();
					m_mousePressed = false;
				}

				// Un-selection
				if( m_mouseReleased && !ptr->m_selectionIsToggle )
					ptr->Deselect();
			} );

			m_mouseReleased = false;
		}

		void InteractableSystem::Render( sf::RenderTarget& target, sf::RenderStates states ) const
		{
			sf::RenderStates copied_states( states );

			//ForEachSystemComponent< Transform, Interactable, SFMLObject >(
			//	[&]( const TransformHandle& transform, InteractableHandle& interactable, const SFMLObjectHandle& sfmlObj )
			//{
			//	if( interactable->m_renderCollisionBounds )
			//	{
			//		copied_states.transform = states.transform * transform->getTransform();
			//
			//		switch( sfmlObj->GetType() )
			//		{
			//		case Components::Rectangle:
			//			target.draw( object->GetRectangleShape(), copied_states );
			//		break;
			//		case Components::Convex:
			//			target.draw( object->GetConvexShape(), copied_states );
			//		break;
			//		case Components::Circle:
			//			target.draw( object->GetCircleShape(), copied_states );
			//		break;
			//		case Components::Sprite:
			//			target.draw( object->GetSprite(), copied_states );
			//		break;
			//		}
			//	}
			//} );
		}

		void InteractableSystem::ProcessEvent( const sf::Event& event )
		{
			if( event.type == sf::Event::MouseButtonPressed )
			{
				m_mousePressed = true;
			}
			else if( event.type == sf::Event::MouseButtonReleased )
			{
				m_mousePressed = false;
				m_mouseReleased = true;
			}
		}
	}
}