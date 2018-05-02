#pragma once

#include <SFML\System\NonCopyable.hpp>
#include <SFML\System\Clock.hpp>
#include <string>
#include <iostream>
#include <memory>
#include <sstream>

#include "VectorMap.h"

namespace Reflex
{
	// Logging system
	inline void LOG_CRIT( std::string log ) { std::cout << "CRIT: " << log << "\n"; }
	inline void LOG_WARN( std::string log ) { std::cout << "Warning: " << log << "\n"; }
	inline void LOG_INFO( std::string log ) { std::cout << "Info: " << log << "\n"; }

	// Profiling code
	namespace Core
	{
		class Profiler : sf::NonCopyable
		{
		public:
			Profiler();
			static Profiler& GetProfiler();
			void StartProfile( const std::string& name );
			void EndProfile( const std::string& name );
			void FrameTick( const sf::Int64 frameTimeMS );
			void OutputResults( const std::string& file );

		protected:

		private:
			struct ProfileData
			{
				sf::Clock timer;
				sf::Int64 currentFrame = 0;
				sf::Int64 shortestFrame = std::numeric_limits< int >::infinity();
				sf::Int64 longestFrame = 0;
				sf::Int64 totalDuration = 0;
				unsigned minHitCount = 1U;
				unsigned maxHitCount = 1U;
				unsigned currentHitCount = 1U;
				unsigned totalSamples = 0U;
			};

			sf::Int64 m_totalDuration = 0;

			Reflex::VectorMap< std::string, ProfileData > m_profileData;
			static std::unique_ptr< Profiler > s_profiler;
		};

		class ScopedProfiler : sf::NonCopyable
		{
		public:
			ScopedProfiler( const std::string& name );
			~ScopedProfiler();

		private:
			const std::string m_profileName;
		};
	}

	#define PROFILE Reflex::Core::ScopedProfiler profile( __FUNCTION__ );
	#define PROFILE_NAME( x ) Reflex::Core::ScopedProfiler profile( x );
}