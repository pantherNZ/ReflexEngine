#pragma once

namespace Reflex
{
	namespace Core
	{
		struct Handle
		{
			unsigned index : 16;
			unsigned innerID : 16;
		};
	}
}