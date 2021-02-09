#include "modules/demo.hpp"

// static Channel<Vector2> playerPosition = {};

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
			this->actors[i] = Vector2{
				static_cast<float>(32 * i),
				static_cast<float>(32 * i)
			};
		}

		if (ona->imageLoad(allocator, "./actor.bmp", &actorImage)) {
			this->actorMaterial = ona->materialCreate(&actorImage);

			ona->imageFree(&actorImage);
		}
	}

	void Process(OnaEvents const * events, OnaContext const * ona) {
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

		// actors[0] = Vector2Add(actors[0], playerPosition.Await());
	}

	void Exit(OnaContext const * ona) {
		ona->materialFree(&this->actorMaterial);
	}
};

struct PlayerController {
	void Init(OnaContext const * ona) {

	}

	void Process(OnaEvents const * events, OnaContext const * ona) {
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

		// playerPosition.Pass(velocity);
	}

	void Exit(OnaContext const * ona) {

	}
};

extern "C" void OnaInit(OnaContext const * ona) {
	ona->spawnSystem(SystemInfoOf<SceneController>());
	ona->spawnSystem(SystemInfoOf<PlayerController>());
}
