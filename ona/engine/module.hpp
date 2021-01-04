#ifndef ENGINE_H
#define ENGINE_H

#include "ona/core/module.hpp"
#include "ona/collections/module.hpp"

namespace Ona::Engine {
	using namespace Ona::Collections;
	using namespace Ona::Core;

	struct Events;

	class FileServer;

	struct File {
		/**
		 * Bitflags used for indicating the access state of a `File`.
		 */
		enum OpenFlags {
			OpenUnknown = 0,
			OpenRead = 0x1,
			OpenWrite = 0x2
		};

		FileServer * server;

		void * userdata;
	};

	class FileServer : public Object {
		public:
		/**
		 * Checks that the file at `filePath` is accessible to the program.
		 *
		 * An inaccessible file may not exist or may be outside of the process permissions to read.
		 */
		virtual bool CheckFile(String const & filePath) = 0;

		/**
		 * Closes the file access referenced by `file`.
		 *
		 * Closing a closed `file` or `file` that does not belong to the `FileServer` does nothing.
		 */
		virtual void CloseFile(File & file) = 0;

		/**
		 * Attempts to open the file at `filePath` into `file` using `openFlags` for access
		 * permissions.
		 */
		virtual bool OpenFile(String const & filePath, File & file, File::OpenFlags openFlags) = 0;

		/**
		 * Attempts to get a `File` connected to the standard output.
		 *
		 * The returned file *does not need to be* and *should not be* freed by the callsite.
		 *
		 * An empty `File` may be returned if the `FileServer` does not support output files at the
		 * time or at any time. It is safe to attempt to write to and read from it in any case.
		 */
		virtual File OutFile() = 0;

		/**
		 * Attempts to print the contents of `string` to the file at `file`.
		 *
		 * Printing to a closed `file` or `file` that does not belong to the `FileServer` does
		 * nothing.
		 *
		 * While the operation may fail, the API deems failure to print `String`s as unimportant and
		 * therefore does not expose any error handling for it.
		 */
		virtual void Print(File & file, String const & string) = 0;

		/**
		 * Attempts to read the contents of the file at `file` into the buffer pointer to in
		 * `output`.
		 *
		 * Reading from a closed `file` or `file` that does not belong to the `FileServer` does
		 * nothing.
		 *
		 * The number of bytes actually read into `output` are returned.
		 */
		virtual size_t Read(File & file, Slice<uint8_t> output) = 0;

		/**
		 * Attempts to seek `offset` bytes into the file at `file` from the file beginning.
		 *
		 * Seeking a closed `file` or `file` that does not belong to the `FileServer` does nothing.
		 *
		 * The number of bytes actually sought are returned.
		 */
		virtual int64_t SeekHead(File & file, int64_t offset) = 0;

		/**
		 * Attempts to seek `offset` bytes into the file at `file` from the file end.
		 *
		 * Seeking a closed `file` or `file` that does not belong to the `FileServer` does nothing.
		 *
		 * The number of bytes actually sought are returned.
		 */
		virtual int64_t SeekTail(File & file, int64_t offset) = 0;

		/**
		 * Attempts to seek `offset` bytes into the file at `file` from the current file cursor
		 * position.
		 *
		 * Seeking a closed `file` or `file` that does not belong to the `FileServer` does nothing.
		 *
		 * The number of bytes actually sought are returned.
		 */
		virtual int64_t Skip(File & file, int64_t offset) = 0;

		/**
		 * Attempts to write the contents of the buffer pointed to in `input` into the contents of
		 * `file`.
		 *
		 * Writing to a closed `file` or `file` that does not belong to the `FileServer` does
		 * nothing.
		 *
		 * The number of bytes actually written to the file are returned.
		 */
		virtual size_t Write(File & file, Slice<uint8_t const> const & input) = 0;
	};

	/**
	 * Attempts to load access to the operating system filesystem as a `FileServer`.
	 */
	FileServer * LoadFilesystem();

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

	struct Sprite {
		Vector3 origin;

		Color tint;
	};

	struct Material;

	class GraphicsQueue : public Object {
		public:
		virtual void RenderSprite(Material * material, Sprite const & sprite) = 0;
	};

	class GraphicsServer : public Object {
		public:
		/**
		 * Clears the backbuffer to black.
		 */
		virtual void Clear() = 0;

		/**
		 * Clears the backbuffer to the color value of `color`.
		 */
		virtual void ColoredClear(Color color) = 0;

		/**
		 * Acquires a thread-local `GraphicsQueue` instance for dispatching graphics operations
		 * asynchronously.
		 *
		 * The contents of the returned `GraphicsQueue` should be automatically dispatched during
		 * `GraphicsServer::Update`.
		 */
		virtual GraphicsQueue * AcquireQueue() = 0;

		/**
		 * Creates a `Material` from the pixel data in `image`.
		 */
		virtual Material * CreateMaterial(Image const & image) = 0;

		/**
		 * Attempts to delete the `Material` from the server located at `material`.
		 */
		virtual void DeleteMaterial(Material * & material) = 0;

		/**
		 * Reads any and all event information known to the `GraphicsServer` at the current frame
		 * and writes it to `events`.
		 *
		 * If the exit signal is received, `false` is returned. Otherwise, `true` is returned to
		 * indicate continued processing.
		 */
		virtual bool ReadEvents(Events * events) = 0;

		/**
		 * Updates the internal state of the `GraphicsServer` and displays the new framebuffer.
		 */
		virtual void Update() = 0;
	};

	GraphicsServer * LoadOpenGl(String const & title, int32_t width, int32_t height);

	#include "ona/api.h"
}

#include "ona/engine/header/config.hpp"

#endif
