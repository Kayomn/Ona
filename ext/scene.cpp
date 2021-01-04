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

	void Init(OnaContext const * ona) {
		Image actorImage;
		Allocator * allocator = ona->defaultAllocator();
		this->graphicsQueue = ona->graphicsQueueAcquire();

		for (size_t i = 0; i < ActorsMax; i += 1) {
			this->actors[i] = Vector2{ona->randomF32(0, 640), ona->randomF32(0, 480)};
		}

		if (ona->imageLoadBitmap(allocator, "./actor.bmp", &actorImage)) {
			this->actorMaterial = ona->materialCreate(&actorImage);

			ona->imageFree(&actorImage);
		}
	}

	void Process(Events const * events, OnaContext const * ona) {
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
	}

	void Exit(OnaContext const * ona) {
		ona->materialFree(&this->actorMaterial);
	}
};

struct PlayerController {
	Vector2 playerPosition;

	void Init(OnaContext const * ona) {

	}

	void Process(Events const * events, OnaContext const * ona) {
		float const deltaSpeed = (events->deltaTime * 0.25f);

		if (events->keysHeld[KeyW]) {
			// this->actors[0].y -= deltaSpeed;
		}

		if (events->keysHeld[KeyA]) {
			// this->actors[0].x -= deltaSpeed;
		}

		if (events->keysHeld[KeyS]) {
			// this->actors[0].y += deltaSpeed;
		}

		if (events->keysHeld[KeyD]) {
			// this->actors[0].x += deltaSpeed;
		}
	}

	void Exit(OnaContext const * ona) {

	}
};

extern "C" void OnaInit(OnaContext const * ona) {
	ona->spawnSystem(SystemInfoOf<SceneController>());
	ona->spawnSystem(SystemInfoOf<PlayerController>());
}
