#include "ona/api.h"
#include <cassert>
#include <cmath>

struct SceneController {
	enum {
		ActorsMax = 32,
	};

	GraphicsQueue * graphicsQueue;

	Material * actorMaterial;

	Vector2 actors[ActorsMax];

	void Init(Context const * ona) {
		Image actorImage;
		Allocator * allocator = ona->defaultAllocator();
		this->graphicsQueue = ona->graphicsQueueCreate();

		for (size_t i = 0; i < ActorsMax; i += 1) {
			this->actors[i] = Vector2{
				std::ceil(ona->randomF32(0, 640)),
				std::ceil(ona->randomF32(0, 480))
			};
		}

		if (ona->imageLoadBitmap(allocator, "./actor.bmp", &actorImage)) {
			this->actorMaterial = ona->materialCreate(&actorImage);

			ona->imageFree(&actorImage);
		}
	}

	void Process(Events const * events, Context const * ona) {
		Sprite actorSprite = {
			.origin = Vector3{},
			.tint = Color{0xFF, 0xFF, 0xFF, 0xFF},
		};

		for (size_t i = 0; i < ActorsMax; i += 1) {
			actorSprite.origin = Vector3{this->actors[i].x, this->actors[i].y};

			ona->renderSprite(this->graphicsQueue, this->actorMaterial, &actorSprite);
		}
	}

	void Exit(Context const * ona) {
		ona->materialFree(&this->actorMaterial);
		ona->graphicsQueueFree(&this->graphicsQueue);
	}
};

struct PlayerController {
	Vector2 playerPosition;

	void Init(Context const * ona) {

	}

	void Process(Events const * events, Context const * ona) {
		float const deltaSpeed = (events->deltaTime * 0.25f);

		if (events->keysHeld[KeyW]) {
			this->playerPosition.y -= deltaSpeed;
		}

		if (events->keysHeld[KeyA]) {
			this->playerPosition.y -= deltaSpeed;
		}

		if (events->keysHeld[KeyS]) {
			this->playerPosition.y += deltaSpeed;
		}

		if (events->keysHeld[KeyD]) {
			this->playerPosition.y += deltaSpeed;
		}
	}

	void Exit(Context const * ona) {

	}
};

extern "C" void OnaInit(Context const * ona) {
	ona->spawnSystem(SystemInfoOf<SceneController>());
	ona->spawnSystem(SystemInfoOf<PlayerController>());
}
