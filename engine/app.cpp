#include "engine.hpp"

using namespace Ona;

using ModuleInitializer = bool(*)(OnaContext const * ona);

struct System {
	void * userdata;

	SystemInitializer initializer;

	SystemProcessor processor;

	SystemFinalizer finalizer;
};

static PackedStack<Library> modules = {DefaultAllocator()};

static PackedStack<System> systems = {DefaultAllocator()};

static GraphicsServer * graphicsServer = nullptr;

static OnaContext const context = {
	.defaultAllocator = DefaultAllocator,

	.graphicsQueueAcquire = []() -> GraphicsQueue * {
		return graphicsServer->AcquireQueue();
	},

	.imageSolid = [](
		Allocator * allocator,
		Point2 dimensions,
		Color color,
		Image * result
	) -> ImageError {
		return Image::Solid(allocator, dimensions, color, *result);
	},

	.imageFree = [](Image * image) {
		image->Free();
	},

	.imageLoad = [](
		Allocator * allocator,
		String const * fileName,
		Image * result
	) -> ImageLoadError {
		return LoadImage(allocator, *fileName, result);
	},

	.materialFree = [](Material * * material) {
		graphicsServer->DeleteMaterial(*material);
	},

	.materialNew = [](Image const * image) -> Material * {
		return graphicsServer->CreateMaterial(*image);
	},

	.renderSprite = [](
		GraphicsQueue * graphicsQueue,
		Material * spriteMaterial,
		Sprite const * sprite
	) {
		graphicsQueue->RenderSprite(spriteMaterial, *sprite);
	},

	.spawnSystem = [](SystemInfo const * info) -> bool {
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

	.stringAssign = [](String * destinationString, char const * value) {
		(*destinationString) = String{value};
	},

	.stringCopy = [](String * destinationString, String const * sourceString) {
		(*destinationString) = (*sourceString);
	},

	.stringDestroy = [](String * string) {
		string->~String();
	},

	.vector2ChannelFree = [](Vector2Channel * * channel) {
		delete (*channel);

		(*channel) = nullptr;
	},

	.vector2ChannelNew = [](Allocator * allocator) -> Vector2Channel * {
		return new Vector2Channel{};
	},

	.vector2ChannelReceive = [](Vector2Channel * channel, Vector2 * output) {
		channel->Receive(*output);
	},

	.vector2ChannelSend = [](Vector2Channel * channel, Vector2 input) {
		channel->Send(input);
	},
};

int main(int argv, char const * const * argc) {
	constexpr Vector2 DisplaySizeDefault = Vector2{640, 480};
	String displayNameDefault = String{"Ona"};
	Allocator * defaultAllocator = DefaultAllocator();
	ConfigEnvironment configEnv = {defaultAllocator};

	{
		Owned<FileContents> fileContents = {};

		if (LoadFile(
			defaultAllocator,
			String{"config.ona"},
			fileContents.value
		) == FileLoadError::None) {
			configEnv.Parse(fileContents.value.ToString());
		}
	}

	RegisterImageLoader(String{"bmp"}, LoadBitmap);
	RegisterGraphicsLoader(String{"opengl"}, LoadOpenGL);

	EnumerateFiles(String{"modules"}, [](String const & fileName) {
		Library moduleLibrary;

		if (OpenLibrary(String::Concat({String{"./"}, fileName}), moduleLibrary)) {
			auto initializer = reinterpret_cast<ModuleInitializer>(
				moduleLibrary.FindSymbol(String{"OnaInit"})
			);

			if (initializer) {
				initializer(&context);

				modules.Push(moduleLibrary);
			} else {
				moduleLibrary.Free();
			}
		}
	});

	{
		String const graphics = {"Graphics"};

		Vector2 const initialDisplaySize = configEnv.ReadVector2({
			graphics,
			String{"displaySize"}
		}, 0, DisplaySizeDefault);

		graphicsServer = LoadGraphics(
			initialDisplaySize.x,
			initialDisplaySize.y,
			configEnv.ReadString({graphics, String{"displayTitle"}}, 0, displayNameDefault),
			configEnv.ReadString({graphics, String{"server"}}, 0, String{"opengl"})
		);
	}

	if (graphicsServer) {
		TaskScheduler tasks = {defaultAllocator, 0.25f};
		OnaEvents events = {};

		systems.ForEach([](System const & system) {
			if (system.initializer) system.initializer(system.userdata, &context);
		});

		while (graphicsServer->ReadEvents(&events)) {
			graphicsServer->Clear();

			systems.ForEach([&](System const & system) {
				tasks.Execute([&]() {
					system.processor(system.userdata, &context, &events);
				});
			});

			tasks.Wait();
			graphicsServer->Update();
		}

		systems.ForEach([](System const & system) {
			if (system.finalizer) system.finalizer(system.userdata, &context);

			DefaultAllocator()->Deallocate(system.userdata);
		});
	}

	modules.ForEach([](Library & moduleLibrary) {
		moduleLibrary.Free();
	});
}
