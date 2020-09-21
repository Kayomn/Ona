#include "engine/graphics.hpp"
#include "collections/appender.hpp"

using namespace Ona::Graphics;
using namespace Ona::Collections;

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
