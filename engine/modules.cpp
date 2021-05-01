#include "engine.hpp"

#include <dlfcn.h>

namespace Ona {
	static OnaContext const context = {
		.acquireGraphicsQueue = [](GraphicsServer * graphicsServer) -> GraphicsQueue * {
			return graphicsServer->AcquireQueue();
		},

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

		.freeChannel = [](Channel * * channel) {
			CloseChannel(*channel);
		},

		.freeImage = [](Image * image) {
			image->Free();
		},

		.freeMaterial = [](GraphicsServer * graphicsServer, Material * * material) {
			graphicsServer->DeleteMaterial(*material);
		},

		.loadImageFile = LoadImage,

		.loadImageSolid = Image::Solid,

		.loadMaterialImage = [](
			GraphicsServer * graphicsServer,
			Image const * image
		) -> Material * {
			return graphicsServer->CreateMaterial(*image);
		},

		.localGraphicsServer = []() {
			return localGraphicsServer;
		},

		.openChannel = OpenChannel,

		.renderSprite = [](
			GraphicsQueue * graphicsQueue,
			Material * spriteMaterial,
			Sprite const * sprite
		) {
			graphicsQueue->RenderSprite(spriteMaterial, *sprite);
		},

		.spawnSystem = [](void * module, OnaSystemInfo const * systemInfo) -> bool {
			return reinterpret_cast<NativeModule *>(module)->SpawnSystem(*systemInfo);
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

	NativeModule::NativeModule(
		Allocator allocator,
		String const & libraryPath
	) : systems{allocator} {
		this->handle = dlopen(libraryPath.ZeroSentineled().Chars().pointer, RTLD_LAZY);

		if (this->handle) {
			this->initializer =
					reinterpret_cast<decltype(initializer)>(dlsym(this->handle, "OnaInit"));

			this->finalizer = reinterpret_cast<decltype(finalizer)>(dlsym(this->handle, "OnaExit"));
		}
	}

	struct NativeModule::System {
		Slice<uint8_t> userdata;

		OnaSystemInitializer initializer;

		OnaSystemProcessor processor;

		OnaSystemFinalizer finalizer;
	};

	NativeModule::~NativeModule() {
		this->systems.ForEach([allocator = this->systems.AllocatorOf()](System const & system) {
			Deallocate(allocator, system.userdata.pointer);
		});

		dlclose(this->handle);
	}

	void NativeModule::Finalize() {
		this->systems.ForEach([](System const & system) {
			system.finalizer(system.userdata.pointer, &context);
		});

		if (this->finalizer) {
			this->finalizer(&context);
		}
	}

	void NativeModule::Initialize() {
		if (this->initializer) {
			this->initializer(&context);
		}
	}

	void NativeModule::Process(OnaEvents const & events) {
		this->systems.ForEach([&events](System const & system) {
			ScheduleTask([&]() {
				system.processor(system.userdata.pointer, &context, &events);
			});
		});
	}

	bool NativeModule::SpawnSystem(OnaSystemInfo const & systemInfo) {
		uint8_t * systemData = Allocate(this->systems.AllocatorOf(), systemInfo.size);

		if (systemData) {
			System * system = this->systems.Push(System{
				.userdata = Slice<uint8_t>{
					.length = systemInfo.size,
					.pointer = systemData,
				},

				.initializer = systemInfo.initializer,
				.processor = systemInfo.processor,
				.finalizer = systemInfo.finalizer,
			});

			if (system) {
				system->initializer(system->userdata.pointer, &context);

				return true;
			}
		}

		return false;
	}
}
