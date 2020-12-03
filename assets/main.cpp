#include "ona/api.h"

extern "C" bool OnaInit(Ona_API const * api) {
	struct TestSystem {
		// SpriteRenderer * renderer;

		// Sprite sprite;
	};

	Ona_SystemInfo testSystemInfo = {
		.size = sizeof(TestSystem),

		.init = [](void * userdata) -> bool {
			// TestSystem * testSystem = ((TestSystem *)userdata);
			// Image image = Image::Solid(DefaultAllocator(), Point2{32, 32}, RGB(0xFF, 0xFF, 0xFF)).Value();
			// testSystem->renderer = SpriteRenderer::Acquire(world->GraphicsServerOf());
			// testSystem->sprite = testSystem->renderer->CreateSprite(world->GraphicsServerOf(), image).Value();

			// image.Free();

			return true;
		},

		.process = [](void * userdata) {
			// TestSystem * testSystem = ((TestSystem *)userdata);
			// testSystem->renderer->Draw(testSystem->sprite, Vector2{10, 10});
		},

		.exit = [](void * userdata) {

		},
	};

	api->spawnSystem(&testSystemInfo);

	return true;
}

// extern "C" void OnaExit(Ona_API const * api) {

// }
