#include "engine/graphics.hpp"

using namespace Ona::Graphics;

int main(int argv, char const * const * argc) {
	GraphicsServer * graphics = LoadOpenGl(String::From("Ona"), 640, 480);

	if (graphics) {
		Events events = {};

		while (graphics->ReadEvents(events)) {
			graphics->Clear();
			graphics->Update();
		}
	}
}
