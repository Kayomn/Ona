#include "ona/engine/module.hpp"

using namespace Ona::Core;
using namespace Ona::Collections;
using namespace Ona::Engine;

using ModuleInitializer = bool(*)(Context const * ona);

struct System {
	void * userdata;

	decltype(SystemInfo::init) initializer;

	decltype(SystemInfo::process) processor;

	decltype(SystemInfo::exit) finalizer;
};

static PackedStack<Library> extensions = {DefaultAllocator()};

static PackedStack<System> systems = {DefaultAllocator()};

static GraphicsServer * graphicsServer = nullptr;

static Context const context = {
	.spawnSystem = [](SystemInfo const * info) {
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

	.graphicsQueueCreate = []() -> GraphicsQueue * {
		return graphicsServer->CreateQueue();
	},

	.graphicsQueueFree = [](GraphicsQueue * * queue) {
		graphicsServer->DeleteQueue(*queue);
	},

	.imageSolid = [](
		Allocator * allocator,
		Point2 dimensions,
		Color color,
		Image * result
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

	.imageFree = [](Image * image) {
		image->Free();
	},

	.imageLoadBitmap = [](
		Allocator * allocator,
		char const * fileName,
		Image * result
	) -> bool {
		Result<Image, ImageError> imageResult = LoadBitmap(allocator, String{fileName});

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

	.materialCreate = [](Image const * image) -> Material * {
		return graphicsServer->CreateMaterial(*image);
	},

	.materialFree = [](Material * * material) {
		graphicsServer->DeleteMaterial(*material);
	},

	.renderSprite = [](
		GraphicsQueue * graphicsQueue,
		Material * spriteMaterial,
		Sprite const * sprite
	) {
		graphicsQueue->RenderSprite(spriteMaterial, *sprite);
	},
};

int main(int argv, char const * const * argc) {
	Allocator * defaultAllocator = DefaultAllocator();
	graphicsServer = LoadOpenGl(String{"Ona"}, 640, 480);

	if (graphicsServer) {
		LuaConfig config = {};

		if (config.Load(Owned<FileContents>{
			LoadFile(defaultAllocator, String{"config.lua"})
		}.value.ToString())) {
			Owned<ConfigValue> extensionNames = config.ReadGlobal(String{"Extensions"});
			uint32_t const extensionsCount = config.ValueLength(extensionNames.value);
			Events events = {};

			for (uint32_t i = 0; i < extensionsCount; i += 1) {
				String moduleName = config.ValueString(Owned<ConfigValue>{
					config.ReadArray(extensionNames.value, i)
				}.value);

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
