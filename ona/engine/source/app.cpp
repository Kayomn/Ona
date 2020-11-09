#include "ona/engine/module.hpp"

namespace Ona::Engine {
	struct System {
		Slice<uint8_t> userdata;

		SystemProcessor processor;

		SystemFinalizer finalizer;
	};

	static PackedStack<System> loadedSystems = {DefaultAllocator()};

	static void(* moduleInitializer)(GraphicsServer * graphicsServer);

	static void(* moduleFinalizer)();
}

int main(int argv, char const * const * argc) {
	using namespace Ona::Core;
	using namespace Ona::Collections;
	using namespace Ona::Engine;

	Result<File, FileOpenError> configFile = OpenFile(
		String::From("./main.lua"),
		File::OpenRead
	);

	if (configFile.IsOk()) {
		LuaEngine lua = {DefaultAllocator()};

		if (lua.ExecuteSourceFile(Slice<LuaVar>{}, configFile.Value()).Length() == 0) {
			LuaVar config = lua.GetGlobal(String::From("Config"));

			if (lua.VarType(config) == LuaType::Table) {
				GraphicsServer * graphicsServer = LoadOpenGl(String::From("Ona"), 640, 480);
				LuaVar nativeModule = lua.GetField(config, String::From("module"));
				Result<Library> loadedModuleLibrary = OpenLibrary(lua.VarString(nativeModule));

				if (loadedModuleLibrary.IsOk()) {
					Library moduleLibrary = loadedModuleLibrary.Value();

					moduleInitializer = reinterpret_cast<decltype(moduleInitializer)>(
						moduleLibrary.FindSymbol(String::From("OnaModuleInit"))
					);

					moduleFinalizer = reinterpret_cast<decltype(moduleFinalizer)>(
						moduleLibrary.FindSymbol(String::From("OnaModuleExit"))
					);
				}

				if (graphicsServer) {
					Events events = {};

					while (graphicsServer->ReadEvents(&events)) {
						graphicsServer->Clear();

						loadedSystems.ForValues([&events](System & system) {
							if (system.processor) {
								system.processor(system.userdata.pointer, &events);
							}
						});

						graphicsServer->Update();
					}
				}
			}
		}

		configFile.Value().Free();
	}
}
