#ifndef ENGINE_H
#define ENGINE_H

#include "ona/core.hpp"

namespace Ona::Graphics {
	using Ona::Core::Color;
	using Ona::Core::String;
	using Ona::Core::Vector4;

	struct Events {
		float deltaTime;
	};

	struct GraphicsCommands {

	};

	class GraphicsServer {
		public:
		virtual ~GraphicsServer() { }

		virtual void Clear() = 0;

		virtual void ColoredClear(Color color) = 0;

		virtual void SubmitCommands(GraphicsCommands const & commands) = 0;

		virtual bool ReadEvents(Events & events) = 0;

		virtual void Update() = 0;
	};

	GraphicsServer * LoadOpenGl(String const & title, int32_t width, int32_t height);

	void UnloadGraphics(GraphicsServer *& graphicsServer);

	Vector4 NormalizeColor(Color const & color);
}

#endif
