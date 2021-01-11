#include "engine/exports.hpp"

#include "dlfcn.h"

using namespace Ona::Core;
using namespace Ona::Collections;
using namespace Ona::Engine;

using ExtensionInitializer = bool(*)(OnaContext const * ona);

struct System {
	void * userdata;

	decltype(SystemInfo::init) initializer;

	decltype(SystemInfo::process) processor;

	decltype(SystemInfo::exit) finalizer;
};

static PackedStack<System> systems = {DefaultAllocator()};

static HashTable<String, ImageLoader> imageLoaders = {DefaultAllocator()};

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
		return IsOk(Image::Solid(allocator, dimensions, color, *result));
	},

	.imageFree = [](Image * image) {
		image->Free();
	},

	.imageLoad = [](Allocator * allocator, char const * fileName, Image * result) -> bool {
		String fileNameString = {fileName};
		ImageLoader const * imageLoader = imageLoaders.Lookup(fileNameString);

		if (imageLoader) {
			Owned<File> file = {};

			if (OpenFile(String{fileName}, File::OpenRead, file.value)) {
				return IsOk((*imageLoader)(allocator, &file.value, result));
			}
		}

		return false;
	},

	.materialCreate = [](Image const * image) -> Material * {
		return graphicsServer->CreateMaterial(*image);
	},

	.materialFree = [](Material * * material) {
		graphicsServer->DeleteMaterial(*material);
	},

	.registerImageLoader = [](char const * fileExtension, ImageLoader imageLoader) -> bool {
		return (imageLoaders.Insert(String{fileExtension}, imageLoader) != nullptr);
	},

	.renderSprite = [](
		GraphicsQueue * graphicsQueue,
		Material * spriteMaterial,
		Sprite const * sprite
	) {
		graphicsQueue->RenderSprite(spriteMaterial, *sprite);
	},
};

static String LoadText(Allocator * tempAllocator, File & file) {
	int64_t const fileCursor = file.Skip(0);
	int64_t const fileSize = file.SeekTail(0);

	file.SeekHead(fileCursor);

	DynamicArray<char> fileBuffer = {tempAllocator, static_cast<size_t>(fileSize)};

	if (fileBuffer.Length()) {
		file.Read(fileBuffer.Values().Bytes());

		return String{fileBuffer.Values()};
	}

	return String{};
}

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
	String configSource = {};
	File configFile;

	if (OpenFile(String{"config.lua"}, File::OpenRead, configFile)) {
		configSource = LoadText(defaultAllocator, configFile);
	}

	if (configSource.Length()) {
		Async async = {defaultAllocator, 0.25f};
		LuaConfig config = {Print};

		if (
			(imageLoaders.Insert(String{"bmp"}, LoadBitmap) != nullptr) &&
			config.Load(configSource)
		) {
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
						auto initializer = reinterpret_cast<ExtensionInitializer>(
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

					systems.ForValues([&events, &async](System const & system) {
						async.Execute([&system, &events]() {
							system.processor(system.userdata, &events, &context);
						});
					});

					async.Wait();
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
