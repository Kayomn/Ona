#include "ona/engine/module.hpp"

namespace Ona::Engine {
	bool World::SpawnSystem(GraphicsServer * graphicsServer, SystemInfo const & systemInfo) {
		Slice<uint8_t> systemUserdata = DefaultAllocator()->Allocate(systemInfo.userdataSize);

		if (systemUserdata.length) {
			if (systemInfo.initializer) {
				if (!systemInfo.initializer(systemUserdata.pointer, graphicsServer)) return false;
			}

			return (this->systems.Push(System{
				.userdata = systemUserdata,
				.processor = systemInfo.processor,
				.finalizer = systemInfo.finalizer
			}) != nullptr);
		}

		return false;
	}

	void World::Update(Events const & events) {
		this->systems.ForValues([&events](System & system) {
			system.processor(system.userdata.pointer, &events);
		});
	}
}
