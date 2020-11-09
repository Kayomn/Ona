#include "ona/engine/module.hpp"

using namespace Ona::Core;
using namespace Ona::Engine;
using namespace Ona::Collections;

int main(int argv, char const * const * argc) {
	Allocator * defaultAllocator = DefaultAllocator();
	GraphicsServer * graphicsServer = LoadOpenGl(String::From("Ona"), 640, 480);

	if (graphicsServer) {
		Events events = {};
		World world = {defaultAllocator};

		struct TestSystem {
			SpriteRenderer * renderer;

			Sprite sprite;
		};

		world.SpawnSystem(graphicsServer, World::SystemInfo{
			.userdataSize = sizeof(TestSystem),

			.initializer = [](void * userdata, GraphicsServer * graphicsServer) {
				TestSystem * system = reinterpret_cast<TestSystem *>(userdata);
				system->renderer = SpriteRenderer::Acquire(graphicsServer);

				if (system->renderer) {
					Image image = Image::Solid(DefaultAllocator(), Point2{32, 32}, RGB(0xFF, 0xFF, 0xFF)).Value();
					system->sprite = system->renderer->CreateSprite(graphicsServer, image).Value();

					image.Free();

					return true;
				}

				return false;
			},

			.processor = [](void * userdata, Events const * events) {
				TestSystem * system = reinterpret_cast<TestSystem *>(userdata);

				system->renderer->Draw(system->sprite, Vector2{100, 100});
			},

			.finalizer = [](void * userdata) {
				TestSystem * system = reinterpret_cast<TestSystem *>(userdata);

				system->renderer->DestroySprite(system->sprite);
			},
		});

		while (graphicsServer->ReadEvents(&events)) {
			graphicsServer->Clear();
			world.Update(events);
			graphicsServer->Update();
		}
	}
}
