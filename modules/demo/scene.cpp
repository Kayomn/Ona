#include "modules/demo.hpp"

static Vector2Channel * playerPositionChannel;

struct SceneController {
	enum {
		ActorsMax = 32,
	};

	GraphicsQueue * graphicsQueue;

	Material * actorMaterial;

	Vector2 actors[ActorsMax];

	void Init(OnaContext const * ona) {
		Image actorImage = {};
		Allocator * allocator = ona->defaultAllocator();
		this->graphicsQueue = ona->graphicsQueueAcquire();

		for (size_t i = 0; i < ActorsMax; i += 1) {
			this->actors[i] = Vector2{
				static_cast<float>(32 * i),
				static_cast<float>(32 * i)
			};
		}

		String filePath = {};

		ona->stringAssign(&filePath, "./actor.bmp");

		if (ona->imageLoad(allocator, &filePath, &actorImage)) {
			this->actorMaterial = ona->materialNew(&actorImage);

			ona->imageFree(&actorImage);
		}

		ona->stringDestroy(&filePath);
	}

	void Process(OnaContext const * ona, OnaEvents const * events) {
		Sprite actorSprite = {
			.origin = Vector3{},
			.tint = Color{0xFF, 0xFF, 0xFF, 0xFF},
		};

		for (size_t i = 0; i < ActorsMax; i += 1) {
			actorSprite.origin = Vector3{
				std::ceil(this->actors[i].x),
				std::ceil(this->actors[i].y)
			};

			ona->renderSprite(this->graphicsQueue, this->actorMaterial, &actorSprite);
		}

		ona->vector2ChannelReceive(playerPositionChannel, &this->actors[0]);
	}

	void Exit(OnaContext const * ona) {
		ona->materialFree(&this->actorMaterial);
	}
};

struct PlayerController {
	void Init(OnaContext const * ona) {

	}

	void Process(OnaContext const * ona, OnaEvents const * events) {
		float const deltaSpeed = (events->deltaTime * 0.25f);
		Vector2 velocity = {};

		if (events->keysHeld[KeyW]) {
			velocity.y -= deltaSpeed;
		}

		if (events->keysHeld[KeyA]) {
			velocity.x -= deltaSpeed;
		}

		if (events->keysHeld[KeyS]) {
			velocity.y += deltaSpeed;
		}

		if (events->keysHeld[KeyD]) {
			velocity.x += deltaSpeed;
		}

		ona->vector2ChannelSend(playerPositionChannel, velocity);
	}

	void Exit(OnaContext const * ona) {

	}
};

extern "C" void OnaInit(OnaContext const * ona) {
	SystemInfo const sceneControllerInfo = {
		.size = sizeof(SceneController),

		.init = [](void * system, OnaContext const * ona) {
			reinterpret_cast<SceneController *>(system)->Init(ona);
		},

		.process = [](void * system, OnaContext const * ona, OnaEvents const * events) {
			reinterpret_cast<SceneController *>(system)->Process(ona, events);
		},

		.exit = [](void * system, OnaContext const * ona) {
			reinterpret_cast<SceneController *>(system)->Exit(ona);
		},
	};

	SystemInfo const playerControllerInfo = {
		.size = sizeof(PlayerController),

		.init = [](void * system, OnaContext const * ona) {
			reinterpret_cast<PlayerController *>(system)->Init(ona);
		},

		.process = [](void * system, OnaContext const * ona, OnaEvents const * events) {
			reinterpret_cast<PlayerController *>(system)->Process(ona, events);
		},

		.exit = [](void * system, OnaContext const * ona) {
			reinterpret_cast<PlayerController *>(system)->Exit(ona);
		},
	};

	playerPositionChannel = ona->vector2ChannelNew(ona->defaultAllocator());
	ona->spawnSystem(&sceneControllerInfo);
	ona->spawnSystem(&playerControllerInfo);
}

extern "C" void OnaExit(OnaContext const * ona) {
	ona->vector2ChannelFree(&playerPositionChannel);
}
