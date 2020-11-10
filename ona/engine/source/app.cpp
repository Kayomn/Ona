#include "ona/engine/module.hpp"

int main(int argv, char const * const * argc) {
	using namespace Ona::Core;
	using namespace Ona::Collections;
	using namespace Ona::Engine;

	struct System {

	};

	Result<String, FileOpenError> mainLuaSource = LoadText(String::From("main.lua"));

	if (mainLuaSource.IsOk()) {
		Allocator * defaultAllocator = DefaultAllocator();
		PackedStack<System> loadedSystems = {defaultAllocator};
		LuaEngine lua = {defaultAllocator};
		ScriptVar ona = lua.NewObject();

		lua.WriteGlobal(String::From("Ona"), ona);

		if (lua.ExecuteSource(mainLuaSource.Value()).IsOk()) {
			GraphicsServer * graphicsServer = LoadOpenGl(String::From("Ona"), 640, 480);

			if (graphicsServer) {
				ScriptVar initializer = ona.ReadObject(String::From("init"));
				ScriptVar processor = ona.ReadObject(String::From("process"));
				ScriptVar finalizer = ona.ReadObject(String::From("exit"));
				Events events = {};

				if (initializer.type == ScriptVar::Type::Callable) initializer.Call();

				while (graphicsServer->ReadEvents(&events)) {
					graphicsServer->Clear();

					if (processor.type == ScriptVar::Type::Callable) processor.Call();

					graphicsServer->Update();
				}

				if (finalizer.type == ScriptVar::Type::Callable) finalizer.Call();
			}
		}
	}
}
