#include <fstream>

#include "Logging.h"
#include "Precompiled.h"
#include <iomanip>

namespace Reflex
{
	namespace Core
	{
		// Static profiler
		std::unique_ptr< Profiler > Profiler::s_profiler = nullptr;

		Profiler::Profiler()
		{
			if( s_profiler )
				LOG_CRIT( "Profiler should not be allocated, please use \"GetProfiler\" static function" );
		}

		// Function definitions
		Profiler& Profiler::GetProfiler()
		{
			if( !s_profiler )
				s_profiler = std::make_unique< Profiler >();
			return *s_profiler.get();
		}

		void Profiler::StartProfile( const std::string& name )
		{
			const auto found = m_profileData.find( name );

			if( found == m_profileData.end() )
			{
				ProfileData data;
				data.timer.restart();
				m_profileData.insert( std::make_pair( name, data ) );
			}
			else
			{
				found->second.currentHitCount++;
				found->second.timer.restart();
			}
		}

		void Profiler::EndProfile( const std::string& name )
		{
			const auto found = m_profileData.find( name );
			assert( found != m_profileData.end() );
			const auto duration = found->second.timer.getElapsedTime().asMicroseconds();
			found->second.currentFrame += duration;
		}

		void Profiler::FrameTick( const sf::Int64 frameTime )
		{
			if( m_profileData.empty() )
				return;

			m_totalDuration += frameTime;

			for( auto& data : m_profileData )
			{
				data.second.minHitCount = std::min( data.second.minHitCount, data.second.currentHitCount );
				data.second.maxHitCount = std::max( data.second.minHitCount, data.second.currentHitCount );
				data.second.totalSamples++;
				data.second.currentHitCount = 1U;

				data.second.shortestFrame = std::min( data.second.shortestFrame, data.second.currentFrame );
				data.second.longestFrame = std::max( data.second.longestFrame, data.second.currentFrame );
				data.second.totalDuration += data.second.currentFrame;

				if( data.second.averageFrame == 0 )
					data.second.averageFrame = data.second.currentFrame;
				else
				{
					const unsigned totalSamples = data.second.totalSamples;
					data.second.averageFrame = data.second.averageFrame * ( totalSamples - 1 ) / totalSamples + data.second.currentFrame / totalSamples;
				}

				data.second.currentFrame = 0;
			}
		}

		void Profiler::OutputResults( const std::string& file )
		{
			std::ofstream stream( file );

			stream << "********* ReflexEngine Performance Logging System **********\n\n";
			const int width = 20;

			//stream << std::setiosflags( std::ios::left ) << std::setw( 40 ) << "Function" << std::resetiosflags( std::ios::left )
			//	<< std::setw( width ) << "Average" << std::setw( width ) << "Min" << std::setw( width ) << "Max"
			//	<< std::setw( width ) << "Percent of Total" << std::setw( width ) << "Min Count" << std::setw( width ) << "Max Count\n";

			for( auto& data : m_profileData )
			{
				stream << std::setprecision( 2 ) << std::fixed << std::setiosflags( std::ios::left ) << std::setw( 40 ) << data.first << std::resetiosflags( std::ios::left )
					<< std::setw( width ) << "Average: " << ( data.second.averageFrame / 1000.0f ) << "ms"
					<< std::setw( width ) << "Min: " << ( data.second.shortestFrame / 1000.0f ) << "ms"
					<< std::setw( width ) << "Max: " << ( data.second.longestFrame / 1000.0f ) << "ms"
					<< std::setw( width ) << "% of Total: " << ( ( data.second.totalDuration / 1000.0f ) / ( m_totalDuration / 1000.0f ) ) << "%"
					<< std::setw( width ) << "Min count: " << data.second.minHitCount
					<< std::setw( width ) << "Max count: " << data.second.maxHitCount << "\n";
			}

			stream.close();
		}

		ScopedProfiler::ScopedProfiler( const std::string& name )
			: m_profileName( name )
		{
			Profiler::GetProfiler().StartProfile( name );
		}

		ScopedProfiler::~ScopedProfiler()
		{
			Profiler::GetProfiler().EndProfile( m_profileName );
		}
	}
}