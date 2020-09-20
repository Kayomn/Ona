#include "engine/graphics.hpp"

using Ona::Math::Vector4;

namespace Ona::Graphics {
	GraphicsServer::~GraphicsServer() { }

	Vector4 NormalizeColor(Color const & color) {
		return Vector4{
			(color.r / (static_cast<float>(Color::channelMax))),
			(color.g / (static_cast<float>(Color::channelMax))),
			(color.b / (static_cast<float>(Color::channelMax))),
			(color.a / (static_cast<float>(Color::channelMax)))
		};
	}
}
