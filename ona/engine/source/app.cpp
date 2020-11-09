#include "ona/engine/module.hpp"

using namespace Ona::Core;
using namespace Ona::Engine;
using namespace Ona::Collections;

int main(int argv, char const * const * argc) {
	Allocator * defaultAllocator = DefaultAllocator();
	GraphicsServer * graphicsServer = LoadOpenGl(String::From("Ona"), 640, 480);
	SpriteRenderer * spriteRenderer = SpriteRenderer::Acquire(graphicsServer);

	if (graphicsServer && spriteRenderer) {
		Events events = {};
		Image image = Image::Solid(defaultAllocator, Point2{32, 32}, RGB(0xFF, 0xFF, 0xFF)).Value();
		Sprite sprite = spriteRenderer->CreateSprite(graphicsServer, image).Value();

		while (graphicsServer->ReadEvents(&events)) {
			graphicsServer->Clear();
			spriteRenderer->Draw(sprite, Vector2{10, 10});
			graphicsServer->Update();
		}
	}
}
