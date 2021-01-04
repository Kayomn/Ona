#include "ona/engine/module.hpp"

#include "dlfcn.h"

using namespace Ona::Core;
using namespace Ona::Collections;
using namespace Ona::Engine;

using ModuleInitializer = bool(*)(OnaContext const * ona);

struct System {
	void * userdata;

	decltype(SystemInfo::init) initializer;

	decltype(SystemInfo::process) processor;

	decltype(SystemInfo::exit) finalizer;
};

static PackedStack<System> systems = {DefaultAllocator()};

static FileServer * fileServer = nullptr;

static GraphicsServer * graphicsServer = nullptr;

static OnaContext const context = {
	.spawnSystem = [](SystemInfo const * info) {
		void * userdata = DefaultAllocator()->Allocate(info->size);

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

	.graphicsQueueAcquire = []() -> GraphicsQueue * {
		return graphicsServer->AcquireQueue();
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
		Result<Image, ImageError> imageResult = LoadBitmap(allocator, fileServer, String{fileName});

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

	.randomF32 = [](float min, float max) -> float {
		return (min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / (max - min)));
	},

	.renderSprite = [](
		GraphicsQueue * graphicsQueue,
		Material * spriteMaterial,
		Sprite const * sprite
	) {
		graphicsQueue->RenderSprite(spriteMaterial, *sprite);
	},
};

static GraphicsServer * LoadGraphicsServerFromConfig(Config * config) {
	Owned<ConfigValue> displaySize = {config->ReadGlobal(String{"DisplaySize"})};

	int64_t const displayWidth = config->ValueInteger(
		Owned<ConfigValue>{config->ReadArray(displaySize.value, 0)}.value
	);

	int64_t const displayHeight = config->ValueInteger(
		Owned<ConfigValue>{config->ReadArray(displaySize.value, 1)}.value
	);

	if ((displayWidth < INT32_MAX) && (displayHeight < INT32_MAX)) {
		return LoadOpenGl(
			config->ValueString(
				Owned<ConfigValue>{config->ReadGlobal(String{"DisplayTitle"})}.value
			),
			static_cast<int32_t>(displayWidth),
			static_cast<int32_t>(displayHeight)
		);
	}

	return nullptr;
}

int main(int argv, char const * const * argc) {
	Allocator * defaultAllocator = DefaultAllocator();
	fileServer = LoadFilesystem();

	LuaConfig config = {[](String const & message) {
		File outFile = fileServer->OutFile();

		fileServer->Print(outFile, message);
	}};

	if (fileServer) {
		String configSource = LoadText(defaultAllocator, fileServer, String{"config.lua"});

		if (configSource.Length()) {
			srand(time(nullptr));

			if (config.Load(configSource)) {
				graphicsServer = LoadGraphicsServerFromConfig(&config);

				if (graphicsServer) {
					PackedStack<void *> extensions = {DefaultAllocator()};
					Owned<ConfigValue> extensionNames = config.ReadGlobal(String{"Extensions"});
					uint32_t const extensionsCount = config.ValueLength(extensionNames.value);
					Events events = {};

					for (uint32_t i = 0; i < extensionsCount; i += 1) {
						String moduleName = config.ValueString(Owned<ConfigValue>{
							config.ReadArray(extensionNames.value, i)
						}.value);

						void * moduleLibrary = dlopen(String::Concat({
							moduleName,
							String{".so"}
						}).ZeroSentineled().Chars().pointer, RTLD_LAZY);

						if (moduleLibrary) {
							auto initializer = reinterpret_cast<ModuleInitializer>(
								dlsym(moduleLibrary, "OnaInit")
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

					extensions.ForValues([](void * extension) {
						dlclose(extension);
					});
				}
			}
		}
	}
}
