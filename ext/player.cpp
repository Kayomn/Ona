#include "ona/api.h"

using namespace Ona::Core;

struct PlayerController {
	Ona_Material * playerSprite;

	Ona::Core::Vector2 playerPosition;

	Ona_GraphicsQueue * graphicsQueue;

	void Init(Ona_CoreContext const * core) {
		Ona_Image image;

		assert(core->imageSolid(
			core->defaultAllocator(),
			Ona_Point2{32, 32},
			Ona_Color{0xFF, 0, 0, 0xFF},
			&image
		));

		this->playerSprite = core->materialCreate(&image);
		this->graphicsQueue = core->graphicsQueueCreate();
		assert(this->playerSprite);
		assert(this->graphicsQueue);
		core->imageFree(&image);
	}

	void Process(Ona_Events const * events, Ona_GraphicsContext const * graphics) {
		float const deltaSpeed = (events->deltaTime * 0.5f);

		if (events->keysHeld[Ona_W]) {
			this->playerPosition.y = Floor(this->playerPosition.y - deltaSpeed);
		}

		if (events->keysHeld[Ona_A]) {
			this->playerPosition.x = Floor(this->playerPosition.x - deltaSpeed);
		}

		if (events->keysHeld[Ona_S]) {
			this->playerPosition.y = Floor(this->playerPosition.y + deltaSpeed);
		}

		if (events->keysHeld[Ona_D]) {
			this->playerPosition.x = Floor(this->playerPosition.x + deltaSpeed);
		}

		Ona_Vector3 position = {this->playerPosition.x, this->playerPosition.y, 0.0f};

		graphics->renderSprite(
			this->graphicsQueue,
			this->playerSprite,
			&position,
			Ona_Color{0xFF, 0xFF, 0xFF, 0xFF}
		);
	}

	void Exit(Ona_CoreContext const * core) {
		core->materialFree(&this->playerSprite);
		core->graphicsQueueFree(&this->graphicsQueue);
	}
};

extern "C" void OnaInit(Ona_CoreContext const * core) {
	core->spawnSystem(Ona_SystemInfoOf<PlayerController>());
}
