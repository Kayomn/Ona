#include "components/core/exports.hpp"

namespace Ona::Core {
	String PathExtension(String const & path) {
		Chars const pathChars = path.Chars();
		uint32_t pathCharsStartIndex = (pathChars.length - 1);
		uint32_t extensionLength = 0;

		while (pathChars.At(pathCharsStartIndex) != '.') {
			pathCharsStartIndex -= 1;
			extensionLength += 1;
		}

		pathCharsStartIndex += 1;

		return String{pathChars.Sliced(pathCharsStartIndex, extensionLength)};
	}
}
