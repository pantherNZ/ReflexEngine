#pragma once

// Includes
#include <memory>
#include <string>
#include <iostream>
#include <assert.h>
#include <stdexcept>
#include <array>
#include <typeindex>
#include <functional>
#include <unordered_map>

#include <SFML/Graphics.hpp>
#include <Box2D.h>

#include "Utility.h"
#include "Logging.h"

// Math common
#define PI					3.141592654f
#define PI2					6.283185307f
#define PIDIV2				1.570796327f
#define PIDIV4				0.785398163f
#define TORADIANS( deg )	( deg ) * PI / 180.0f
#define TODEGREES( rad )	( rad ) / PI * 180.0f