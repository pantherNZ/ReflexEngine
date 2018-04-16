#include "Utility.h"

namespace Reflex
{
	std::vector< std::string > Split( const std::string& _strInput, const char _cLetter )
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

	// Trim from start (in place)
	void TrimLeft( std::string& str )
	{
		str.erase( str.begin(), std::find_if( str.begin(), str.end(), []( char ch )
		{
			return !IsSpace( ch );
		} ) );
	}

	// trim from end (in place)
	void TrimRight( std::string& str )
	{
		str.erase( std::find_if( str.rbegin(), str.rend(), []( char ch )
		{
			return !IsSpace( ch );
		} ).base(), str.end() );
	}

	bool IntersectPolygonCircle( const std::vector< sf::Vector2f >& polygon, const sf::Vector2f& circlePosition, const float radius )
	{
		const size_t polygon_size = polygon.size();
		for( size_t i = 0; i < polygon_size; ++i )
		{
			//We first project the center of the circle onto the line and get the vector from the line to the circle
			const sf::Vector2f lineStartToCircle = circlePosition - polygon[i];
			const sf::Vector2f lineStartToEnd = polygon[( i + 1 ) % polygon_size] - polygon[i];
			const sf::Vector2f fromClosestPointToCircle = lineStartToCircle - lineStartToEnd * Clamp( Dot( lineStartToCircle, lineStartToEnd ) / Dot( lineStartToEnd, lineStartToEnd ) );

			//Calculate distance to circle from this point
			const float distanceSquared = Dot( fromClosestPointToCircle, fromClosestPointToCircle );

			//Check if the sphere is intersecting the line
			if( distanceSquared <= radius * radius )
			{
				return true;
			}

			//If the polygon is on the left it is outside the polygon. No collision.
			const sf::Vector2f lineLeftNormal( lineStartToEnd.y, -lineStartToEnd.x );
			if( Dot( lineLeftNormal, fromClosestPointToCircle ) > 0.0f )
			{
				return false;
			}
		}

		//The circle is on the right of every line. It is inside the polygon.
		return true;
	}

	bool IntersectLineLine( const sf::Vector2f& p1, const sf::Vector2f& p2, const sf::Vector2f& p3, const sf::Vector2f& p4 )
	{
		const sf::Vector2f second_line_segment = p4 - p3;
		const sf::Vector2f first_line_segment = p2 - p1;
		const float denominator = second_line_segment.y * first_line_segment.x - second_line_segment.x * first_line_segment.y;

		if( abs( denominator ) < 0.00001f )
			return false;

		const sf::Vector2f first_line_to_second = p1 - p3;
		const float first_intersection = ( second_line_segment.x * first_line_to_second.y - second_line_segment.y * first_line_to_second.x ) / denominator;
		const float second_intersection = ( first_line_segment.x * first_line_to_second.y - first_line_segment.y * first_line_to_second.x ) / denominator;

		return ( first_intersection >= 0.0f && first_intersection <= 1.0f && second_intersection >= 0.0f && second_intersection <= 1.0f );
	}

	bool IntersectPolygonLine( const std::vector< sf::Vector2f >& polygon, const sf::Vector2f& line_begin, const sf::Vector2f& line_end )
	{
		const size_t polygon_size = polygon.size();
		for( size_t i = 0; i < polygon_size; ++i )
		{
			if( IntersectLineLine( polygon[i], polygon[( i + 1 ) % polygon_size], line_begin, line_end ) )
				return true;
		}
		return false;
	}

	bool IntersectLineCircle( const sf::Vector2f& line_begin, const sf::Vector2f& line_end, const sf::Vector2f& circle_position, const float circle_radius )
	{
		//Get the vector from the closest point on the line to the circle center
		const sf::Vector2f line_start_to_circle = circle_position - line_begin;
		const sf::Vector2f line_start_to_end = line_end - line_begin;
		const sf::Vector2f closest_line_point_to_circle = line_start_to_circle - line_start_to_end * Clamp( Dot( line_start_to_circle, line_start_to_end ) / Dot( line_start_to_end, line_start_to_end ) );

		const float distance_squared = Dot( closest_line_point_to_circle, closest_line_point_to_circle );

		return distance_squared <= circle_radius * circle_radius;
	}

	bool IntersectCircleCircle( const sf::Vector2f& position1, const sf::Vector2f& position2, const float size1, const float size2 )
	{
		const sf::Vector2f distance_vector = position1 - position2;
		const float collision_distance = size1 + size2;
		const float distance_length_squared = Dot( distance_vector, distance_vector );

		return ( distance_length_squared <= collision_distance * collision_distance );
	}

	bool IntersectPolygonSquare( const std::vector< sf::Vector2f >& polygon, const sf::Vector2f& square_position, const float half_width )
	{
		const size_t hull_size = polygon.size();
		for( size_t i = 0; i < hull_size; ++i )
		{
			const sf::Vector2f first_point = polygon[i];
			const sf::Vector2f second_point = polygon[( i + 1 ) % hull_size];
			const sf::Vector2f line_top_left( std::min( first_point.x, second_point.x ), std::min( first_point.y, second_point.y ) );
			const sf::Vector2f line_bottom_right( std::max( first_point.x, second_point.x ), std::max( first_point.y, second_point.y ) );

			if( line_top_left.x <= square_position.x + half_width && 
				line_bottom_right.x >= square_position.x - half_width && 
				line_top_left.y <= square_position.y + half_width && 
				line_bottom_right.y >= square_position.y - half_width )
			{
				return true;
			}
		}
		return false;
	}

	bool IntersectCircleSquare( const sf::Vector2f& circle_position, const float circle_radius, const sf::Vector2f& square_position, const float half_width )
	{
		const float left = square_position.x - half_width;
		const float top = square_position.y - half_width;
		const float right = square_position.x + half_width;
		const float bottom = square_position.y + half_width;

		const sf::Vector2f polygon[5] =
		{
			sf::Vector2f( left, top ),
			sf::Vector2f( right, top ),
			sf::Vector2f( right, bottom ),
			sf::Vector2f( left, bottom ),
			sf::Vector2f( left, top )
		};

		for( unsigned i = 0; i < 4; ++i ) //-V112
		{
			//We first project the center of the circle onto the line and get the vector from the line to the circle
			const sf::Vector2f lineStartToCircle = circle_position - polygon[i];
			const sf::Vector2f lineStartToEnd = polygon[i + 1] - polygon[i];
			const sf::Vector2f fromClosestPointToCircle = lineStartToCircle - lineStartToEnd * Clamp( Dot( lineStartToCircle, lineStartToEnd ) / Dot( lineStartToEnd, lineStartToEnd ) );

			//Calculate distance to circle from this point
			const float distanceSquared = Dot( fromClosestPointToCircle, fromClosestPointToCircle );

			//Check if the sphere is intersecting the line
			if( distanceSquared <= circle_radius * circle_radius )
			{
				return true;
			}

			//If the polygon is on the left it is outside the polygon. No collision.
			const sf::Vector2f lineLeftNormal( lineStartToEnd.y, -lineStartToEnd.x );
			if( Dot( lineLeftNormal, fromClosestPointToCircle ) > 0.0f )
			{
				return false;
			}
		}

		//The circle is on the right of every line. It is inside the polygon.
		return true;
	}
}