#pragma once

// Includes
#include <memory>
#include <string>
#include <iostream>
#include <assert.h>
#include <stdexcept>
#include <array>

#include <SFML/Graphics.hpp>

#include "Math.h"

namespace
{
	const std::string logTitle[3] = 
	{ 
		"Log: ", 
		"Warning: ", 
		"Critical: " 
	};
}

// Common Utility
namespace Reflex
{
	enum class ELogType : unsigned char
	{
		LOG,
		WARN,
		CRIT,
	};

	inline void LOG( const ELogType _type, std::string _message )
	{
		std::cout << logTitle[( int )_type];
		std::cout << _message << "\n";
		assert( _type != ELogType::CRIT && _message.c_str() );
	}

	inline void LOG_CRIT( std::string _message ) { LOG( ELogType::CRIT, _message ); }
	inline void LOG_WARN( std::string _message ) { LOG( ELogType::WARN, _message ); }
	inline void LOG_INFO( std::string _message ) { LOG( ELogType::LOG, _message ); }

	inline float RandomFloat()
	{
		return rand() / ( RAND_MAX + 1.0f );
	}

	inline float Rand( const float _fMax )
	{
		return Rand( 0.0f, _fMax );
	}

	inline float Rand( const float _fMin, const float _fMax )
	{
		return _fMin + RandomFloat() * ( _fMax - _fMin );
	}

	inline float Round( float _fVal, float _fAccuracy )
	{
		_fVal *= _fAccuracy;
		return( floor( _fVal + 0.5f ) / _fAccuracy );
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
}