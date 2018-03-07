#pragma once

#include "ResourceManager.h"

// Common Utility
namespace Reflex
{
	namespace Core
	{
		// Type defs
		typedef ResouceManager< sf::Texture > TextureManager;
		typedef ResouceManager< sf::Font > FontManager;
		typedef std::type_index Type;
	}

	#define STRINGIFY( x ) #x
	#define TOSTRING( x ) STRINGIFY( x )
	#define TODO( Msg ) \
		__pragma( message( __FILE__ "(" TOSTRING( __LINE__ ) ") : TODO [ " Msg " ]" ) )

	inline float RandomFloat()
	{
		return rand() / ( RAND_MAX + 1.0f );
	}

	inline float Rand( const float _fMin, const float _fMax )
	{
		return _fMin + RandomFloat() * ( _fMax - _fMin );
	}

	inline float Rand( const float _fMax )
	{
		return Rand( 0.0f, _fMax );
	}

	inline float Round( float _fVal, int _iAccuracy )
	{
		_fVal *= _iAccuracy;
		return( std::floor( _fVal + 0.5f ) / ( float )_iAccuracy );
	}

	static inline std::vector< std::string > Split( const std::string& _strInput, const char _cLetter )
	{
		std::vector< std::string > vecReturn;
		std::string strWriter;

		for( unsigned i = 0; i < _strInput.size(); ++i )
		{
			if( _strInput[i] != _cLetter )
			{
				strWriter += _strInput[i];
			}
			else
			{
				vecReturn.push_back( strWriter );
				strWriter = "";
			}
		}

		if( strWriter.size() )
			vecReturn.push_back( strWriter );

		return std::move( vecReturn );
	}

	static inline bool IsSpace( const char c )
	{
		return c == ' ' || c == '\t';
	}

	// Trim from start (in place)
	static inline void TrimLeft( std::string& str )
	{
		str.erase( str.begin(), std::find_if( str.begin(), str.end(), []( char ch )
		{
			return !IsSpace( ch );
		} ) );
	}

	// trim from end (in place)
	static inline void TrimRight( std::string& str )
	{
		str.erase( std::find_if( str.rbegin(), str.rend(), []( char ch )
		{
			return !IsSpace( ch );
		} ).base(), str.end() );
	}

	// trim from both ends (in place)
	static inline void Trim( std::string &str )
	{
		TrimLeft( str );
		TrimRight( str );
	}

	// Centre the origin of an SFML object (such as Sprite, Text, Shape etc.)
	template< class T >
	static inline void CenterOrigin( T& object )
	{
		sf::FloatRect bounds = object.getLocalBounds();
		object.setOrigin( std::floor( bounds.left + bounds.width / 2.f ), std::floor( bounds.top + bounds.height / 2.f ) );
	}
}