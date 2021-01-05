#include "ona/engine/module.hpp"

namespace Ona::Engine {
	void File::Free() {
		this->server->CloseFile(*this);
	}
}
