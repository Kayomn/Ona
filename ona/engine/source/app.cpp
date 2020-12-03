#include "ona/engine/module.hpp"

#include "ona/api.h"

using namespace Ona::Core;
using namespace Ona::Collections;
using namespace Ona::Engine;

using ModuleInitializer = bool(*)(Ona_API const * api);

struct System {
	void * userdata;

	decltype(Ona_SystemInfo::init) initializer;

	decltype(Ona_SystemInfo::process) processor;

	decltype(Ona_SystemInfo::exit) finalizer;
};

static PackedStack<Library> modules = {DefaultAllocator()};

static PackedStack<System> systems = {DefaultAllocator()};

static bool SpawnSystem(Ona_SystemInfo const * info) {
	void * userdata = DefaultAllocator()->Allocate(info->size).pointer;

	if (userdata && systems.Push(System{
		.userdata = userdata,
		.initializer = info->init,
		.processor = info->process,
		.finalizer = info->exit,
	})) {
		return true;
	}

	return false;
}

int main(int argv, char const * const * argc) {
	Allocator * defaultAllocator = DefaultAllocator();
	GraphicsServer * graphicsServer = LoadOpenGl(String{"Ona"}, 640, 480);

	if (graphicsServer) {
		LuaEngine lua = {defaultAllocator};

		LuaVar configTable = lua.ExecuteScript(
			Unique<FileContents>{LoadFile(defaultAllocator, String{"config.lua"})}.value.ToString()
		);

		if (configTable.type == LuaType::Table) {
			LuaVar moduleTable = lua.ReadTable(configTable, String{"modules"});

			if (moduleTable.type == LuaType::Table) {
				size_t const moduleTableLength = lua.VarLength(moduleTable);
				Events events = {};

				Ona_API const api = {
					.spawnSystem = SpawnSystem,
				};

				for (size_t i = 0; i < moduleTableLength; i += 1) {
					Library moduleLibrary = OpenLibrary(lua.VarIndex(moduleTable, i).ToString());

					if (moduleLibrary.IsOpen()) {
						auto initializer = reinterpret_cast<ModuleInitializer>(
							moduleLibrary.FindSymbol(String{"OnaInit"})
						);

						if (initializer) initializer(&api);

						modules.Push(moduleLibrary);
					}
				}

				systems.ForValues([](System const & system) {
					if (system.initializer) system.initializer(system.userdata);
				});

				while (graphicsServer->ReadEvents(&events)) {
					graphicsServer->Clear();

					systems.ForValues([](System const & system) {
						if (system.processor) system.processor(system.userdata);
					});

					graphicsServer->Update();
				}

				systems.ForValues([](System const & system) {
					if (system.finalizer) system.finalizer(system.userdata);
				});

				modules.ForValues([](Library & module_) {
					module_.Free();
				});
			}
		}
	}
}
