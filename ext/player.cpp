#include "ona/api.h"
#include <cassert>
#include <cmath>

struct PlayerController {
	Material * playerSprite;

	Vector2 playerPosition;

	GraphicsQueue * graphicsQueue;

	void Init(Context const * ona) {
		Image image;

		assert(ona->imageLoadBitmap(ona->defaultAllocator(), "./icon.bmp", &image));

		this->playerSprite = ona->materialCreate(&image);
		this->graphicsQueue = ona->graphicsQueueCreate();

		assert(this->playerSprite);
		assert(this->graphicsQueue);
		ona->imageFree(&image);
	}

	void Process(Events const * events, Context const * ona) {
		float const deltaSpeed = (events->deltaTime * 0.5f);

		if (events->keysHeld[W]) {
			this->playerPosition.y = std::ceil(this->playerPosition.y - deltaSpeed);
		}

		if (events->keysHeld[A]) {
			this->playerPosition.x = std::ceil(this->playerPosition.x - deltaSpeed);
		}

		if (events->keysHeld[S]) {
			this->playerPosition.y = std::ceil(this->playerPosition.y + deltaSpeed);
		}

		if (events->keysHeld[D]) {
			this->playerPosition.x = std::ceil(this->playerPosition.x + deltaSpeed);
		}

		Sprite zeroInfo = {
			.origin = Vector3{0, 0, 0},
			.tint = Color{0xFF, 0, 0, 0xFF}
		};

		Sprite player = {
			.origin = Vector3{this->playerPosition.x, this->playerPosition.y},
			.tint = Color{0xFF, 0xFF, 0xFF, 0xFF},
		};

		ona->renderSprite(this->graphicsQueue, this->playerSprite, &zeroInfo);
		ona->renderSprite(this->graphicsQueue, this->playerSprite, &player);
	}

	void Exit(Context const * ona) {
		ona->materialFree(&this->playerSprite);
		ona->graphicsQueueFree(&this->graphicsQueue);
	}
};

extern "C" void OnaInit(Context const * ona) {
	ona->spawnSystem(SystemInfoOf<PlayerController>());
}
