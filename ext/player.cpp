#include "ona/api.h"

struct PlayerController {
	Ona_Material * playerSprite;

	Ona_Vector2 playerPosition;

	void Init(Ona_CoreContext const * core) {
		Ona_Image image;

		assert(core->imageSolid(
			core->defaultAllocator(),
			Ona_Point2{32, 32},
			Ona_Color{0xFF, 0xFF, 0xFF, 0xFF},
			&image
		));

		this->playerSprite = core->createMaterial(&image);
		assert(this->playerSprite);
		core->imageFree(&image);
	}

	void Process(Ona_Events const * events, Ona_GraphicsContext const * graphics) {
		if (events->keysHeld[Ona_W]) {
			this->playerPosition.y -= events->deltaTime;
		}

		if (events->keysHeld[Ona_A]) {
			this->playerPosition.x -= events->deltaTime;
		}

		if (events->keysHeld[Ona_S]) {
			this->playerPosition.y += events->deltaTime;
		}

		if (events->keysHeld[Ona_D]) {
			this->playerPosition.x += events->deltaTime;
		}

		Ona_Vector3 position = {this->playerPosition.x, this->playerPosition.y, 0.0f};

		graphics->renderSprite(this->playerSprite, &position, Ona_Color{0xFF, 0xFF, 0xFF, 0xFF});
	}

	void Exit(Ona_CoreContext const * core) {

	}
};

extern "C" void OnaInit(Ona_CoreContext const * core) {
	core->spawnSystem(Ona_SystemInfoOf<PlayerController>());
}
