#include "ona/engine/module.hpp"

int main(int argv, char const * const * argc) {
	using namespace Ona::Core;
	using namespace Ona::Collections;
	using namespace Ona::Engine;

	struct System {

	};

	Allocator * defaultAllocator = DefaultAllocator();
	PackedStack<System> loadedSystems = {defaultAllocator};
	LuaEngine main = {defaultAllocator};

	if (main.ExecuteSource(Unique<FileContents>{
		LoadFile(defaultAllocator, String::From("main.lua"))
	}.value.ToString()) == ScriptState::Ok) {
		GraphicsServer * graphicsServer = LoadOpenGl(String::From("Ona"), 640, 480);

		if (graphicsServer) {
			Events events = {};

			main.CallInitializer();

			while (graphicsServer->ReadEvents(&events)) {
				graphicsServer->Clear();

				loadedSystems.ForValues([](System & system) {

				});

				graphicsServer->Update();
			}

			main.CallFinalizer();
		}
	}
}
