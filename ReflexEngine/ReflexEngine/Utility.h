#pragma once

#include <SFML\Graphics\Texture.hpp>
#include <SFML\Graphics\Font.hpp>
#include <SFML\Graphics\Sprite.hpp>

#include <sstream>
#include <typeindex>

// Common Utility
namespace Reflex
{
	namespace Core
	{
		typedef std::type_index Type;
	}

	// Math common
	#define PI					3.141592654f
	#define PI2					6.283185307f
	#define PIDIV2				1.570796327f
	#define PIDIV4				0.785398163f
	#define TORADIANS( deg )	( deg ) * PI / 180.0f
	#define TODEGREES( rad )	( rad ) / PI * 180.0f

	// String common
	#define STRINGIFY( x ) #x
	#define STRINGIFY2( x ) STRINGIFY( x )
	#define TODO( Msg ) __pragma( message( __FILE__ "(" STRINGIFY2( __LINE__ ) ") : TODO [ " Msg " ]" ) )
	#define Stream( message ) [&](){ std::stringstream s; s << message; return s.str(); }()

	// Bounding box with rect
	struct BoundingBox : public sf::FloatRect
	{
		// Methods
		BoundingBox();
		BoundingBox( const sf::FloatRect& aabb, const float rotation = 0.0f );

		bool contains( float x, float y ) const;
		bool contains( const sf::Vector2f& point ) const;
		bool intersects( const BoundingBox& other ) const;

		// Members
		float rotation = 0.0f;
	};

	// Circle struct
	struct Circle
	{
		// Methods
		Circle();
		Circle( const sf::Vector2f& centre, const float radius );

		bool Contains( const sf::Vector2f& position ) const;
		bool Intersects( const Circle& other ) const;

		// Members
		sf::Vector2f centre;
		float radius;
	};

	// Helper functions
	inline int RandomInt( const int max )
	{
		return rand() % ( max + 1 );
	}

	inline int RandomInt( const int min, const int max )
	{
		return min + ( RandomInt( max - min ) );
	}

	inline bool RandomBool()
	{
		return rand() > ( RAND_MAX / 2 );
	}

	inline float RandomFloat()
	{
		return rand() / ( RAND_MAX + 1.0f );
	}

	inline float RandomFloat( const float min, const float max )
	{
		return min + RandomFloat() * ( max - min );
	}

	inline float RandomFloat( const float max )
	{
		return RandomFloat( 0.0f, max );
	}

	inline float Round( float value )
	{
		return std::floor( value + 0.5f );
	}

	inline float Round( float value, int accuracy )
	{
		value *= accuracy;
		return( std::floor( value + 0.5f ) / ( float )accuracy );
	}

	inline int RoundToInt( float value )
	{
		return ( int )std::floor( value + 0.5f );
	}

	template< typename T >
	inline T Clamp( T x, T min, T max )
	{
		return std::min( max, std::max( min, x ) );
	}

	template< typename T >
	inline T Clamp( T x )
	{
		return std::min( ( T )1.0, std::max( ( T )0.0, x ) );
	}

	template< typename T >
	inline T Sign( T x )
	{
		return T( ( double )x > 0.0f ? 1.0 : ( double )x < 0.0 ? -1.0f : 0.0f );
	}

	// String functions
	std::vector< std::string > Split( const std::string& _strInput, const char _cLetter );

	inline bool IsSpace( const char c )
	{
		return c == ' ' || c == '\t';
	}

	// Trim from start (in place)
	void TrimLeft( std::string& str );

	// trim from end (in place)
	void TrimRight( std::string& str );

	// trim from both ends (in place)
	inline void Trim( std::string &str )
	{
		TrimLeft( str );
		TrimRight( str );
	}

	template< typename CharType, typename T >
	std::basic_string< CharType > ToBasicString( const T& t )
	{
		std::basic_stringstream< CharType > stream;
		stream << t;
		return stream.str();
	}

	template< typename T >
	std::string ToString( const T& t )
	{
		return ToBasicString< char >( t );
	}

	template< typename T, typename ... Args >
	std::string ToString( const T& t, const Args& ... args )
	{
		return ToString( t ) + ToString( args... );
	}

	// Centre the origin of an SFML object (such as Sprite, Text, Shape etc.)
	template< class T >
	inline void CenterOrigin( T& object )
	{
		sf::FloatRect bounds = object.getLocalBounds();
		object.setOrigin( std::floor( bounds.left + bounds.width / 2.f ), std::floor( bounds.top + bounds.height / 2.f ) );
	}

	// Awful conversions
	inline sf::Vector2i Vector2fToVector2i( const sf::Vector2f& convert )
	{
		return sf::Vector2i( ( int )Round( convert.x ), ( int )Round( convert.y ) );
	}

	inline sf::Vector2u Vector2fToVector2u( const sf::Vector2f& convert )
	{
		return sf::Vector2u( ( unsigned )Round( convert.x ), ( unsigned )Round( convert.y ) );
	}

	inline sf::Vector2f Vector2iToVector2f( const sf::Vector2i& convert )
	{
		return sf::Vector2f( ( float )convert.x, ( float )convert.y );
	}

	inline sf::Vector2u Vector2iToVector2u( const sf::Vector2i& convert )
	{
		return sf::Vector2u( ( unsigned )convert.x, ( unsigned )convert.y );
	}

	inline sf::Vector2f Vector2uToVector2f( const sf::Vector2u& convert )
	{
		return sf::Vector2f( ( float )convert.x, ( float )convert.y );
	}

	inline sf::Vector2i Vector2uToVector2i( const sf::Vector2u& convert )
	{
		return sf::Vector2i( ( int )convert.x, ( int )convert.y );
	}

	// Useful math functions
	inline float Dot( const sf::Vector2f& a, const sf::Vector2f& b )
	{
		return a.x * b.x + a.y * b.y;
	}

	inline float GetMagnitudeSq( const sf::Vector2f& a )
	{
		return float( a.x * a.x + a.y * a.y );
	}

	inline float GetMagnitude( const sf::Vector2f& a )
	{
		return sqrtf( GetMagnitudeSq( a ) );
	}

	inline float GetDistanceSq( const sf::Vector2f& a, const sf::Vector2f& b )
	{
		return float( GetMagnitudeSq( a - b ) );
	}

	inline float GetDistance( const sf::Vector2f& a, const sf::Vector2f& b )
	{
		return GetMagnitude( a - b );
	}

	inline sf::Vector2f Normalise( const sf::Vector2f& a )
	{
		const auto distance = GetMagnitude( a );
		return a / distance;
	}

	inline sf::Vector2f VectorFromAngle( const float angleDegrees, const float distance )
	{
		const auto angleRadians = TORADIANS( angleDegrees );
		return sf::Vector2f( distance * sin( angleRadians ), distance * -cos( angleDegrees ) );
	}

	inline sf::Vector2f RotateVector( const sf::Vector2f& vector, const float angleDegrees )
	{
		const auto angleRadians = TORADIANS( angleDegrees );
		const float sinR = sin( angleRadians );
		const float cosR = cos( angleRadians );
		return sf::Vector2f( vector.x * cosR - vector.y * sinR, vector.y * cosR + vector.x * sinR );
	}

	inline sf::Vector2f RotateAroundPoint( const sf::Vector2f& position, const sf::Vector2f& rotateAround, const float angleDegrees )
	{
		const auto angleRadians = TORADIANS( angleDegrees );
		const float sinR = sin( angleRadians );
		const float cosR = cos( angleRadians );
		const float diffX = position.x - rotateAround.x;
		const float diffY = position.y - rotateAround.y;
		return sf::Vector2f( cosR * diffX - sinR * diffY + rotateAround.x, sinR * diffX + cosR * diffY + rotateAround.y );
	}

	inline sf::Vector2i RotateAroundPoint( const sf::Vector2i& position, const sf::Vector2i& rotateAround, const float angleDegrees )
	{
		return Vector2fToVector2i( RotateAroundPoint( Vector2iToVector2f( position ), Vector2iToVector2f( rotateAround ), angleDegrees ) );
	}

	inline void ScaleTo( sf::Sprite& sprite, const sf::Vector2f& targetScale )
	{
		sprite.setScale( sf::Vector2f( targetScale.x / ( float )sprite.getTextureRect().width, targetScale.y / ( float )sprite.getTextureRect().height ) );
	}

	bool IntersectPolygonCircle( const std::vector< sf::Vector2f >& polygon, const sf::Vector2f& circlePosition, const float radius );
	bool IntersectPolygonCircle( const std::vector< sf::Vector2f >& polygon, const Circle& circle );

	bool IntersectLineLine( const sf::Vector2f& p1, const sf::Vector2f& p2, const sf::Vector2f& p3, const sf::Vector2f& p4 );

	bool IntersectPolygonLine( const std::vector< sf::Vector2f >& polygon, const sf::Vector2f& line_begin, const sf::Vector2f& line_end );

	bool IntersectLineCircle( const sf::Vector2f& line_begin, const sf::Vector2f& line_end, const sf::Vector2f& circle_position, const float circle_radius );
	bool IntersectLineCircle( const sf::Vector2f& line_begin, const sf::Vector2f& line_end, const Circle& circle );

	bool IntersectCircleCircle( const sf::Vector2f& position1, const float size1, const sf::Vector2f& position2, const float size2 );
	bool IntersectCircleCircle( const Circle& circle1, const Circle& circle2 );

	bool IntersectPolygonSquare( const std::vector< sf::Vector2f >& polygon, const sf::Vector2f& square_position, const float half_width );

	bool IntersectCircleSquare( const sf::Vector2f& circle_position, const float circle_radius, const sf::Vector2f& square_position, const float half_width );

	template< typename T >
	const typename T::value_type& RandomElement(const T& container)
	{
		if (container.empty())
			throw std::runtime_error("RandomElement called on empty container");
		return container[RandomUnsigned((unsigned)container.size())];
	}

	template< typename T >
	typename T::value_type& RandomElement(const T& container)
	{
		if (container.empty())
			throw std::runtime_error("RandomElement called on empty container");
		return container[RandomUnsigned((unsigned)container.size())];
	}

	template< typename T >
	typename std::vector<T>::const_iterator Erase(std::vector<T>& container, const T& value)
	{
		return container.erase(container.begin(), container.end(), value);
	}

	template< typename T, typename Pred >
	typename std::vector<T>::const_iterator EraseIf(std::vector<T>& container, const Pred& pred)
	{
		return container.erase(std::remove_if(container.begin(), container.end(), pred), container.end());
	}

	template< typename T >
	typename std::vector<T>::const_iterator Find(const std::vector<T>& container, const T& value)
	{
		return std::find(container.begin(), container.end(), value);
	}

	template< typename T, typename Pred >
	typename std::vector<T>::const_iterator FindIf(const std::vector<T>& container, const Pred& pred)
	{
		return std::find_if(container.begin(), container.end(), pred);
	}

	template< typename T >
	bool Contains(const std::vector< T >& container, const T& value)
	{
		return Find(container, value) != container.end();
	}

	template< typename T, typename Pred >
	bool ContainsIf(const std::vector< T >& container, const Pred& pred)
	{
		return FindIf(container, pred) != container.end();
	}

	template< typename T, typename... Args >
	bool PushBack(const std::vector< T >& container, Args&&... args)
	{
		return (container.push_back(args), ...);
	}

	// Hash stuff
	template <typename T, typename... Rest>
	void HashCombine(std::size_t& seed, const T& v, const Rest& ... rest)
	{
		seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
		(HashCombine(seed, rest), ...);
	}

#define MAKE_HASHABLE( type, ... ) \
    namespace std \
	{ \
        template<> struct hash<type> \
		{ \
            std::size_t operator()( const type& t) const \
			{ \
                std::size_t ret = 0; \
                Reflex::HashCombine( ret, __VA_ARGS__ ); \
                return ret; \
            } \
        }; \
    }
}

MAKE_HASHABLE(sf::Vector2f, t.x, t.y)
MAKE_HASHABLE(sf::Vector2i, t.x, t.y)
MAKE_HASHABLE(sf::Vector2u, t.x, t.y)