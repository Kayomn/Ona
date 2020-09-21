#include "ona/engine.hpp"

using Ona::Core::Vector4;

namespace Ona::Graphics {
	Vector4 NormalizeColor(Color const & color) {
		return Vector4{
			(color.r / (static_cast<float>(Color::channelMax))),
			(color.g / (static_cast<float>(Color::channelMax))),
			(color.b / (static_cast<float>(Color::channelMax))),
			(color.a / (static_cast<float>(Color::channelMax)))
		};
	}
}
