// Includes
#include "..\ReflexEngine\Engine.h"

#include "GraphState.h"
#include "MenuState.h"
#include "SpacialHashMapDemo.h"
#include "..\ReflexEngine\SFMLObjectComponent.h"

enum StateTypes : unsigned
{
	MenuStateType,
	GraphStateType,
	SpacialHashMapState,
	NumStateTypes,
};

class Velocity : public Reflex::Components::Component
{
public:
	using Reflex::Components::Component::Component;

	sf::Vector2f velocity;
};

class VelocitySystem : public Reflex::Systems::System
{
public:
	using Reflex::Systems::System::System;

	virtual void RegisterComponents()
	{
		RequiresComponent( Reflex::Components::Transform );
		RequiresComponent( Velocity );
	}

	virtual void Update( const float deltaTime ) 
	{ 
		ForEachSystemComponent< Reflex::Components::Transform, Velocity >( []( const auto& t, const auto& v )
			{
				t->move( v->velocity );
			} );
	}

	virtual void Render( sf::RenderTarget& target, sf::RenderStates states ) const { }
};

class TestState : public State
{
public:
	TestState( StateManager& stateManager, Context context )
		: State( stateManager, context )
		, m_bounds( 0.0f, 0.0f, (float )context.window->getSize().x, (float )context.window->getSize().y )
		, m_world( context, m_bounds, 300U )
	{
		//m_world.AddSystem< VelocitySystem >();

		for( unsigned i = 0; i < 1000; ++i )
		{
			auto object = m_world.CreateObject( sf::Vector2f( Reflex::RandomFloat( 0.0f, m_bounds.width ), Reflex::RandomFloat( 0.0f, m_bounds.height ) ) );
			object->AddComponent< Velocity >()->velocity = sf::Vector2f( Reflex::RandomFloat( -5.0f, 5.0f ), Reflex::RandomFloat( -5.0f, 5.0f ) );
			auto shape = sf::CircleShape( 5.0f );
			shape.setFillColor( sf::Color::Red );
			object->AddComponent< Reflex::Components::SFMLObject >( shape );
		}
	}

	virtual bool Update( const float deltaTime ) override
	{
		m_world.Update( deltaTime );
		return true;
	}

	virtual bool ProcessEvent( const sf::Event& event ) override
	{
		m_world.ProcessEvent( event );
		return true;
	}

	virtual void Render() override
	{
		m_world.Render();
	}

	sf::FloatRect m_bounds;
	World m_world;
};



// Entry point
int main( int argc, char** argv )
{
	srand( ( unsigned )time( 0 ) );

	Reflex::Core::Engine engine;

	//engine.RegisterState< SpacialHashMapDemo >( SpacialHashMapState, true );
	//engine.RegisterState< GraphState >( GraphStateType );
	//engine.RegisterState< MenuState >( MenuStateType );

	{
		Reflex::Core::ScopedProfiler profile( "Setup" );
		engine.RegisterState< TestState >( 0, true );
	}

	engine.Run();

	return 0; 
}