#include "ona/engine/module.hpp"

#include "ona/api.h"

using namespace Ona::Core;
using namespace Ona::Collections;
using namespace Ona::Engine;

using ModuleInitializer = bool(*)(Ona_CoreContext const * core);

struct System {
	void * userdata;

	decltype(Ona_SystemInfo::init) initializer;

	decltype(Ona_SystemInfo::process) processor;

	decltype(Ona_SystemInfo::exit) finalizer;
};

static PackedStack<Library> extensions = {DefaultAllocator()};

static PackedStack<System> systems = {DefaultAllocator()};

static GraphicsServer * graphicsServer = nullptr;

static Ona_CoreContext const coreContext = {
	.spawnSystem = [](Ona_SystemInfo const * info) {
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
	},

	.defaultAllocator = DefaultAllocator,

	.imageSolid = [](
		Ona_Allocator * allocator,
		Ona_Point2 dimensions,
		Ona_Color color,
		Ona_Image * result
	) -> bool {
		Result<Image, ImageError> imageResult = Image::Solid(allocator, dimensions, color);

		if (imageResult.IsOk()) {
			if (result) {
				(*result) = imageResult.Value();
			} else {
				imageResult.Value().Free();
			}

			return true;
		}

		return false;
	},

	.imageFree = [](Ona_Image * imageResult) {
		imageResult->Free();
	},

	.createMaterial = [](Ona_Image const * image) -> Ona_Material * {
		return graphicsServer->CreateMaterial(*image);
	},
};

static Ona_GraphicsContext graphicsContext = {
	.renderSprite = [](
		Ona_Material * spriteMaterial,
		Ona_Vector3 const * position,
		Ona_Color tint
	) {
		graphicsServer->RenderSprite(static_cast<Material *>(spriteMaterial), *position, tint);
	},
};

int main(int argv, char const * const * argc) {
	Allocator * defaultAllocator = DefaultAllocator();
	graphicsServer = LoadOpenGl(String{"Ona"}, 640, 480);

	if (graphicsServer) {
		LuaConfig config = {};

		if (config.Load(Unique<FileContents>{
			LoadFile(defaultAllocator, String{"config.lua"})
		}.ValueOf().ToString())) {
			Unique<ConfigValue> extensionNames = config.ReadGlobal(String{"Extensions"});
			uint32_t const extensionsCount = config.ValueLength(extensionNames.ValueOf());
			Events events = {};

			for (uint32_t i = 0; i < extensionsCount; i += 1) {
				String moduleName = config.ValueString(Unique<ConfigValue>{
					config.ReadArray(extensionNames.ValueOf(), i)
				}.ValueOf());

				Library moduleLibrary = OpenLibrary(String::Concat({moduleName, String{".so"}}));

				if (moduleLibrary.IsOpen()) {
					auto initializer = reinterpret_cast<ModuleInitializer>(
						moduleLibrary.FindSymbol(String{"OnaInit"})
					);

					if (initializer) initializer(&coreContext);

					extensions.Push(moduleLibrary);
				}
			}

			systems.ForValues([](System const & system) {
				if (system.initializer) system.initializer(&coreContext, system.userdata);
			});

			while (graphicsServer->ReadEvents(&events)) {
				graphicsServer->Clear();

				systems.ForValues([](System const & system) {
					if (system.processor) system.processor(&graphicsContext, system.userdata);
				});

				graphicsServer->Update();
			}

			systems.ForValues([](System const & system) {
				if (system.finalizer) system.finalizer(&coreContext, system.userdata);
			});

			extensions.ForValues([](Library & module_) {
				module_.Free();
			});
		}
	}
}
