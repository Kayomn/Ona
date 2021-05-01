#include "modules/demo.hpp"

static Channel * playerPositionChannel;

struct SceneController {
	enum {
		ActorsMax = 1024,
	};

	GraphicsServer * graphicsServer;

	GraphicsQueue * graphicsQueue;

	Material * actorMaterial;

	Vector2 actors[ActorsMax];

	void Init(OnaContext const * ona) {
		this->graphicsServer = ona->localGraphicsServer();
		this->graphicsQueue = ona->acquireGraphicsQueue(graphicsServer);

		for (size_t i = 0; i < ActorsMax; i += 1) {
			this->actors[i] = Vector2{
				.x = static_cast<float>(32 * i),
				.y = static_cast<float>(32 * i)
			};
		}

		String systemPath;

		ona->stringAssign(&systemPath, "./Texture.bmp");

		Stream * stream = ona->openSystemStream(&systemPath, StreamAccess_Read);
		Image * actorImage = ona->loadImageBitmap(stream, Allocator_Default);

		ona->stringDestroy(&systemPath);
		ona->freeStream(&stream);

		if (actorImage) {
			this->actorMaterial = ona->loadMaterialImage(this->graphicsServer, actorImage);

			ona->freeImage(actorImage);
		}
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

		Vector2 velocity;

		ona->channelReceive(playerPositionChannel, sizeof(Vector2), &velocity);

		this->actors[0].x += velocity.x;
		this->actors[0].y += velocity.y;
	}

	void Exit(OnaContext const * ona) {
		ona->freeMaterial(this->graphicsServer, &this->actorMaterial);
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

		ona->channelSend(playerPositionChannel, sizeof(Vector2), &velocity);
	}

	void Exit(OnaContext const * ona) {

	}
};

extern "C" void OnaInit(OnaContext const * ona, void * module) {
	OnaSystemInfo const sceneControllerInfo = {
		.size = sizeof(SceneController),

		.initializer = [](void * system, OnaContext const * ona) {
			reinterpret_cast<SceneController *>(system)->Init(ona);
		},

		.processor = [](void * system, OnaContext const * ona, OnaEvents const * events) {
			reinterpret_cast<SceneController *>(system)->Process(ona, events);
		},

		.finalizer = [](void * system, OnaContext const * ona) {
			reinterpret_cast<SceneController *>(system)->Exit(ona);
		},
	};

	OnaSystemInfo const playerControllerInfo = {
		.size = sizeof(PlayerController),

		.initializer = [](void * system, OnaContext const * ona) {
			reinterpret_cast<PlayerController *>(system)->Init(ona);
		},

		.processor = [](void * system, OnaContext const * ona, OnaEvents const * events) {
			reinterpret_cast<PlayerController *>(system)->Process(ona, events);
		},

		.finalizer = [](void * system, OnaContext const * ona) {
			reinterpret_cast<PlayerController *>(system)->Exit(ona);
		},
	};

	playerPositionChannel = ona->openChannel(sizeof(Vector2));
	ona->spawnSystem(module, &sceneControllerInfo);
	ona->spawnSystem(module, &playerControllerInfo);
}

extern "C" void OnaExit(OnaContext const * ona) {
	ona->freeChannel(&playerPositionChannel);
}
