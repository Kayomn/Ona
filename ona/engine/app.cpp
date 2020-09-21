#include "ona/engine.hpp"
#include "ona/collections.hpp"

using namespace Ona::Core;
using namespace Ona::Engine;
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
