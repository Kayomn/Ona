#include "engine.hpp"

namespace Ona {
	static HashTable<String, GraphicsLoader> graphicsLoaders = {DefaultAllocator()};

	static PackedStack<String> graphicsServers = {DefaultAllocator()};

	GraphicsServer * LoadGraphics(
		int32_t displayWidth,
		int32_t displayHeight,
		String const & displayTitle,
		String const & server
	) {
		GraphicsLoader * graphicsLoader = graphicsLoaders.Lookup(server);

		return (
			graphicsLoader ?
			(*graphicsLoader)(displayTitle, displayWidth, displayHeight) :
			nullptr
		);
	}

	void RegisterGraphicsLoader(String const & server, GraphicsLoader graphicsLoader) {
		graphicsServers.Push(server);
		graphicsLoaders.Insert(server, graphicsLoader);
	}
}
