#include "ona/engine/module.hpp"

using namespace Ona::Core;
using namespace Ona::Engine;
using namespace Ona::Collections;

int main(int argv, char const * const * argc) {
	Allocator * allocator = DefaultAllocator();
	GraphicsServer * graphics = LoadOpenGl(String::From("Ona"), 640, 480);
	SpriteCommands * spriteCommands = allocator->New<SpriteCommands>(graphics);

	if (graphics) {
		Image image = Image::Solid(allocator, Point2{32, 32}, RGB(0xFF, 0xFF, 0xFF)).Value();
		Sprite sprite = CreateSprite(graphics, image).Value();
		Events events = Events{};

		while (graphics->ReadEvents(&events)) {
			graphics->Clear();
			spriteCommands->Draw(sprite, Vector2{32, 32});
			spriteCommands->Dispatch(graphics);
			graphics->Update();
		}

		image.Free();
		sprite.Free();
	}

	allocator->Destroy(spriteCommands);
}
