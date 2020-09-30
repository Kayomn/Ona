#include "ona/engine.hpp"
#include "ona/collections.hpp"

using namespace Ona::Core;
using namespace Ona::Engine;
using namespace Ona::Collections;

int main(int argv, char const * const * argc) {
	let graphics = LoadOpenGl(String::From("Ona"), 640, 480);
	let spriteCommands = graphics->CreateCommandBuffer<SpriteCommands>().Value();

	if (graphics.HasValue()) {
		let image = Image::Solid(nil<Allocator *>, Point2{32, 32}, colorWhite).Value();
		let sprite = CreateSprite(graphics.Value(), image).Value();
		let events = Events{};

		while (graphics->ReadEvents(&events)) {
			graphics->Clear();
			spriteCommands->Draw(sprite, Vector2{32, 32});
			spriteCommands->Dispatch(graphics.Value());
			graphics->Update();
		}
	}
}
