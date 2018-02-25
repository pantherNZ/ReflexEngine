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
		typedef std::type_index ComponentType;
	}

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

	inline std::vector< std::string > Split( const std::string& _strInput, const char _cLetter )
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

	inline int IndexOf( const std::string& _strInput, const char _cLetter )
	{
		for( unsigned i = 0; i < _strInput.size(); ++i )
			if( _strInput[i] == _cLetter )
				return i;

		return -1;
	}

	inline bool IsSpace( const char& _cLetter )
	{
		return _cLetter == ' ' || _cLetter == '\t';
	}

	inline std::string TrimEnd( const std::string& _strInput )
	{
		if( _strInput.size() == 0 || !IsSpace( _strInput.back() ) )
			return _strInput;

		auto strReturn( _strInput );
		unsigned int uiCounter = 0;

		for( uiCounter = strReturn.size() - 1; uiCounter >= 0; uiCounter++ )
			if( !IsSpace( strReturn[uiCounter] ) )
				break;

		strReturn.erase( strReturn.begin() + uiCounter, strReturn.end() );
		return strReturn;
	}

	inline std::string TrimStart( const std::string& _strInput )
	{
		if( _strInput.size() == 0 || !IsSpace( _strInput.front() ) )
			return _strInput;

		auto strReturn( _strInput );
		unsigned int uiCounter = 0;

		for( uiCounter = 0; uiCounter < strReturn.size(); uiCounter++ )
			if( !IsSpace( strReturn[uiCounter] ) )
				break;

		strReturn.erase( strReturn.begin(), strReturn.begin() + uiCounter );
		return strReturn;
	}

	inline std::string Trim( const std::string& _strInput )
	{
		return TrimStart( TrimEnd( _strInput ) );
	}

	inline void CenterOrigin( sf::Sprite& sprite )
	{
		sf::FloatRect bounds = sprite.getLocalBounds();
		sprite.setOrigin( std::floor( bounds.left + bounds.width / 2.f ), std::floor( bounds.top + bounds.height / 2.f ) );
	}

	inline void CenterOrigin( sf::Text& text )
	{
		sf::FloatRect bounds = text.getLocalBounds();
		text.setOrigin( std::floor( bounds.left + bounds.width / 2.f ), std::floor( bounds.top + bounds.height / 2.f ) );
	}

	inline void CenterOrigin( sf::CircleShape& circle )
	{
		sf::FloatRect bounds = circle.getLocalBounds();
		circle.setOrigin( std::floor( bounds.left + bounds.width / 2.f ), std::floor( bounds.top + bounds.height / 2.f ) );
	}
}