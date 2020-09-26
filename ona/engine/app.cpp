#include "ona/engine.hpp"
#include "ona/collections.hpp"

using namespace Ona::Core;
using namespace Ona::Engine;
using namespace Ona::Collections;

int main(int argv, char const * const * argc) {
	GraphicsServer * graphics = LoadOpenGl(String::From("Ona"), 640, 480);
	SpriteRenderCommands * spriteCommands = New<SpriteRenderCommands>(graphics);

	if (graphics && spriteCommands) {
		Image image = Image::Solid(nullptr, Point2{32, 32}, Color{255, 255, 255, 255}).Value();
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
