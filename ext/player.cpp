#include "ona/api.h"

using namespace Ona::Core;

struct PlayerController {
	Ona_Material * playerSprite;

	Ona::Core::Vector2 playerPosition;

	Ona_GraphicsQueue * graphicsQueue;

	void Init(Ona_Context const * ona) {
		Ona_Image image;

		assert(ona->imageSolid(
			ona->defaultAllocator(),
			Ona_Point2{32, 32},
			Ona_Color{0xFF, 0, 0, 0xFF},
			&image
		));

		this->playerSprite = ona->materialCreate(&image);
		this->graphicsQueue = ona->graphicsQueueCreate();
		assert(this->playerSprite);
		assert(this->graphicsQueue);
		ona->imageFree(&image);
	}

	void Process(Ona_Events const * events, Ona_Context const * ona) {
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

		ona->renderSprite(
			this->graphicsQueue,
			this->playerSprite,
			&position,
			Ona_Color{0xFF, 0xFF, 0xFF, 0xFF}
		);
	}

	void Exit(Ona_Context const * ona) {
		ona->materialFree(&this->playerSprite);
		ona->graphicsQueueFree(&this->graphicsQueue);
	}
};

extern "C" void OnaInit(Ona_Context const * ona) {
	ona->spawnSystem(Ona_SystemInfoOf<PlayerController>());
}
