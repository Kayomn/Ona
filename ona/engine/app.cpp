#include "ona/engine.hpp"
#include "ona/collections.hpp"

using namespace Ona::Core;
using namespace Ona::Engine;
using namespace Ona::Collections;

int main(int argv, char const * const * argc) {
	let graphics = LoadOpenGl(String::From("Ona"), 640, 480).Value();
	let spriteCommands = graphics->CreateCommandBuffer<SpriteCommands>().Value();

	if (graphics && spriteCommands) {
		Image image = Image::Solid(
			Optional<Allocator *>{},
			Point2{32, 32},
			Color{255, 255, 255, 255}
		).Value();

		Sprite sprite = CreateSprite(graphics, image).Value();
		Events events = {};

		while (graphics->ReadEvents(&events)) {
			graphics->Clear();
			spriteCommands->Draw(sprite, Vector2{32, 32});
			spriteCommands->Dispatch(graphics);
			graphics->Update();
		}
	}
}
