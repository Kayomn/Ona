#include "ona/engine/module.hpp"

using namespace Ona::Core;
using namespace Ona::Collections;
using namespace Ona::Engine;

using ModuleInitializer = bool(*)(Ona_Context const * ona);

struct System {
	void * userdata;

	decltype(Ona_SystemInfo::init) initializer;

	decltype(Ona_SystemInfo::process) processor;

	decltype(Ona_SystemInfo::exit) finalizer;
};

static PackedStack<Library> extensions = {DefaultAllocator()};

static PackedStack<System> systems = {DefaultAllocator()};

static GraphicsServer * graphicsServer = nullptr;

static Ona_Context const context = {
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

	.graphicsQueueCreate = []() -> Ona_GraphicsQueue * {
		return reinterpret_cast<Ona_GraphicsQueue *>(graphicsServer->CreateQueue());
	},

	.graphicsQueueFree = [](Ona_GraphicsQueue * * queue) {
		graphicsServer->DeleteQueue(*reinterpret_cast<GraphicsQueue * *>(queue));
	},

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

	.materialCreate = [](Ona_Image const * image) -> Ona_Material * {
		return reinterpret_cast<Ona_Material *>(graphicsServer->CreateMaterial(*image));
	},

	.materialFree = [](Ona_Material * * material) {
		graphicsServer->DeleteMaterial(*reinterpret_cast<Material * *>(material));
	},

	.renderSprite = [](
		Ona_GraphicsQueue * graphicsQueue,
		Ona_Material * spriteMaterial,
		Ona_Vector3 const * position,
		Ona_Color tint
	) {
		reinterpret_cast<GraphicsQueue *>(graphicsQueue)->RenderSprite(
			reinterpret_cast<Material *>(spriteMaterial),
			*position,
			tint
		);
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

					if (initializer) initializer(&context);

					extensions.Push(moduleLibrary);
				}
			}

			systems.ForValues([](System const & system) {
				if (system.initializer) system.initializer(system.userdata, &context);
			});

			while (graphicsServer->ReadEvents(&events)) {
				graphicsServer->Clear();

				systems.ForValues([&events](System const & system) {
					if (system.processor) {
						system.processor(
							system.userdata,
							&events,
							&context
						);
					}
				});

				graphicsServer->Update();
			}

			systems.ForValues([](System const & system) {
				if (system.finalizer) system.finalizer(system.userdata, &context);

				DefaultAllocator()->Deallocate(system.userdata);
			});

			extensions.ForValues([](Library & module_) {
				module_.Free();
			});
		}
	}
}
