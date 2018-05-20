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

		void InteractableSystem::Update( const float deltaTime )
		{
			const auto mouse_position = Reflex::ToVector2f( sf::Mouse::getPosition( *GetWorld().GetContext().window ) );

			ForEachSystemComponent< Transform, Interactable, SFMLObject >(
				[&]( const TransformHandle& transform, InteractableHandle& interactable, const SFMLObjectHandle& sfmlObj )
			{
				bool collision = false;
				
				// Collision with bounds
				TODO( "Handle collision with rotated rects" );
				switch( sfmlObj->GetType() )
				{
				case SFMLObjectType::Circle:
					collision = Reflex::Circle( transform->GetWorldPosition(), sfmlObj->GetCircleShape().getRadius() ).Contains( mouse_position );
				break;
				case SFMLObjectType::Rectangle:
					collision = Reflex::ToAABB( transform->GetWorldTransform().transformRect( sfmlObj->GetRectangleShape().getLocalBounds() ) ).Contains( mouse_position );
				break;
				case SFMLObjectType::Convex:
					collision = Reflex::ToAABB( transform->GetWorldTransform().transformRect( sfmlObj->GetConvexShape().getLocalBounds() ) ).Contains( mouse_position );
				break;
				case SFMLObjectType::Sprite:
					collision = Reflex::ToAABB( transform->GetWorldTransform().transformRect( sfmlObj->GetSprite().getLocalBounds() ) ).Contains( mouse_position );
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
					if( !collision && !ptr->m_selectionIsToggle )
						ptr->Deselect();
				}

				// Selection (or can be deselection for toggle mode)
				if( ptr->m_isFocussed && m_mousePressed )
					if( ptr->m_isSelected == ptr->m_selectionIsToggle )
						ptr->m_isSelected ? ptr->Deselect() : ptr->Select();

				// Un-selection
				if( m_mouseReleased && !ptr->m_selectionIsToggle )
					ptr->Deselect();
			} );

			m_mouseReleased = false;
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