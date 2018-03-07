#pragma once

#include <sstream>

namespace
{
	const std::string logTitle[3] =
	{
		"Log: ",
		"Warning: ",
		"Critical: "
	};
}

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
	}

#	define Stream( message ) [&](){ std::stringstream s; s << message; return s.str( ); }()

	inline void LOG_CRIT( std::string _message ) { LOG( ELogType::CRIT, _message ); }
	inline void LOG_WARN( std::string _message ) { LOG( ELogType::WARN, _message ); }
	inline void LOG_INFO( std::string _message ) { LOG( ELogType::LOG, _message ); }
}