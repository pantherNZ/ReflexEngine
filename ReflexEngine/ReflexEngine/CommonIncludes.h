#pragma once

// Includes
#include <memory>
#include <string>
#include <iostream>

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

	inline void Log( const ELogType _type, std::string _message )
	{
		std::cout << logTitle[( int )_type];
		std::cout << _message << "\n";
	}
}