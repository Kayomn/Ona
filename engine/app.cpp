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

	.channelClose = [](Channel * * channel) {
		CloseChannel(*channel);
	},

	.channelOpen = OpenChannel,

	.channelReceive = [](
		Channel * channel,
		size_t outputBufferLength,
		void * outputBufferPointer
	) -> uint32_t {
		return ChannelReceive(channel, Slice<uint8_t>{
			.length = outputBufferLength,
			.pointer = reinterpret_cast<uint8_t *>(outputBufferPointer)
		});
	},

	.channelSend = [](
		Channel * channel,
		size_t inputBufferLength,
		void const * inputBufferPointer
	) -> uint32_t {
		return ChannelSend(channel, Slice<uint8_t const>{
			.length = inputBufferLength,
			.pointer = reinterpret_cast<uint8_t const *>(inputBufferPointer)
		});
	},

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
};

int main(int argv, char const * const * argc) {
	constexpr Vector2 DisplaySizeDefault = Vector2{640, 480};
	String displayNameDefault = String{"Ona"};
	Allocator * defaultAllocator = DefaultAllocator();

	RegisterImageLoader(String{"bmp"}, LoadBitmap);
	RegisterGraphicsLoader(String{"opengl"}, LoadOpenGL);

	EnumerateFiles(String{"modules"}, [](String const & fileName) {
		Library moduleLibrary;

		if (OpenLibrary(String::Concat({String{"./modules/"}, fileName}), moduleLibrary)) {
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
		ConfigEnvironment configEnv = {defaultAllocator};
		String const graphics = {"Graphics"};
		Owned<FileContents> fileContents = {};

		if (LoadFile(
			defaultAllocator,
			String{"config.ona"},
			fileContents.value
		) == FileLoadError::None) {
			String errorMessage = {};

			if (configEnv.Parse(
				fileContents.value.ToString(),
				&errorMessage
			) != ScriptError::None) {
				Print(errorMessage);
			}
		}

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
		OnaEvents events = {};

		InitScheduler();

		systems.ForEach([](System const & system) {
			if (system.initializer) system.initializer(system.userdata, &context);
		});

		while (graphicsServer->ReadEvents(&events)) {
			graphicsServer->Clear();

			systems.ForEach([&](System const & system) {
				ScheduleTask([&]() {
					system.processor(system.userdata, &context, &events);
				});
			});

			WaitAllTasks();
			graphicsServer->Update();
		}

		systems.ForEach([defaultAllocator](System const & system) {
			if (system.finalizer) system.finalizer(system.userdata, &context);

			defaultAllocator->Deallocate(system.userdata);
		});
	}

	modules.ForEach([](Library & moduleLibrary) {
		moduleLibrary.Free();
	});
}
