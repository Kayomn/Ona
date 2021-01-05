#ifndef ENGINE_H
#define ENGINE_H

#include "ona/core/module.hpp"
#include "ona/collections/module.hpp"

#include "ona/engine/header/config.hpp"
#include "ona/engine/header/fileserver.hpp"
#include "ona/engine/header/graphics.hpp"
#include "ona/engine/header/os.hpp"

namespace Ona::Engine {
	using namespace Ona::Collections;
	using namespace Ona::Core;

	/**
	 * Loads the text contents of the file at `filePath` from `fileServer` into memory using
	 * `tempAllocator` to allocate the temporary buffer used in between reading it into memory and
	 * converting the memory contents to a `String`.
	 *
	 * Failure to load a file from the given `filePath` and `fileServer` will result in an empty
	 * `String` being returned.
	 */
	String LoadText(Allocator * tempAllocator, FileServer * fileServer, String const & filePath);

	/**
	 * Attempts to load the contents of the file at `filePath` from `fileServer` into memory as a
	 * bitmap using `imageAllocator` as the dynamic memory allocator for the `Image` pixels.
	 *
	 * Failure to load a file from the given `filePath` and `fileServer` will result in a 32x32
	 * placeholder image being allocated and returned instead. A file I/O failure is not considered
	 * an error by `LoadBitmap`, so the only two failstates remain the two represented by
	 * `ImageError`.
	 *
	 * `LoadBitmap` supports 24-bit and 32-bit BGRA-encoded uncompressed bitmaps. Any other formats
	 * will result in `ImageError::UnsupportedFormat`
	 */
	Result<Image, ImageError> LoadBitmap(
		Allocator * imageAllocator,
		FileServer * fileServer,
		String const & filePath
	);

	#include "ona/api.h"
}

#endif
