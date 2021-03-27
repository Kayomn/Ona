#ifndef ONA_ENGINE_H
#define ONA_ENGINE_H

#include "common.hpp"

namespace Ona {
	/**
	 * 32-bit RGBA color value.
	 */
	struct Color {
		uint8_t r, g, b, a;

		/**
		 * Creates a `Vector4` containing each RGBA color channel of the `Color` normalized to a
		 * value between `0` and `1`.
		 */
		Vector4 Normalized() const;
	};

	/**
	 * Creates an opaque greyscale `Color` from `value`, where `0` is absolute black and `0xFF` is
	 * absolute white.
	 */
	constexpr Color Greyscale(uint8_t const value) {
		return Color{value, value, value, 0xFF};
	}

	/**
	 * Creates an opaque RGB `Color` from `red`, `green`, and `blue.
	 */
	constexpr Color RGB(uint8_t const red, uint8_t const green, uint8_t const blue) {
		return Color{red, green, blue, 0xFF};
	}

	enum class ImageError {
		None,
		UnsupportedFormat,
		OutOfMemory,
	};

	enum class ImageLoadError {
		None,
		FileError,
		UnsupportedFormat,
		OutOfMemory,
	};

	/**
	 * Resource handle to a buffer of 32-bit RGBA pixel data.
	 */
	struct Image {
		Allocator * allocator;

		Color * pixels;

		Point2 dimensions;

		void Free();

		static ImageError From(
			Allocator * allocator,
			Point2 dimensions,
			Color * pixels,
			Image & result
		);

		static ImageError Solid(
			Allocator * allocator,
			Point2 dimensions,
			Color color,
			Image & result
		);
	};

	ImageLoadError LoadBitmap(Allocator * imageAllocator, String filePath, Image * imageResult);

	using ImageLoader = ImageLoadError(*)(
		Allocator * allocator,
		String filePath,
		Image * imageResult
	);

	ImageLoadError LoadImage(Allocator * allocator, String const & filePath, Image * imageResult);

	void RegisterImageLoader(String const & fileFormat, ImageLoader imageLoader);

	struct Sprite {
		Vector3 origin;

		Color tint;
	};

	struct Material;

	class GraphicsQueue : public Object {
		public:
		virtual void RenderSprite(Material * material, Sprite const & sprite) = 0;
	};

	struct OnaEvents;

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
		virtual bool ReadEvents(OnaEvents * events) = 0;

		/**
		 * Updates the internal state of the `GraphicsServer` and displays the new framebuffer.
		 */
		virtual void Update() = 0;
	};

	GraphicsServer * LoadOpenGL(String title, int32_t width, int32_t height);

	using GraphicsLoader = GraphicsServer * (*)(
		String displayTitle,
		int32_t displayWidth,
		int32_t displayHeight
	);

	GraphicsServer * LoadGraphics(
		int32_t displayWidth,
		int32_t displayHeight,
		String const & displayTitle,
		String const & server
	);

	/**
	 * Registers `graphicsLoader` as the loader assigned for servers that identify as `server`.
	 */
	void RegisterGraphicsLoader(String const & server, GraphicsLoader graphicsLoader);

	enum class ScriptError {
		None,
		ParsingSyntax,
		OutOfMemory,
	};

	class ConfigEnvironment final : public Object {
		private:
		struct Value;

		HashTable<String, Value> globals;

		public:
		ConfigEnvironment(Allocator * allocator);

		~ConfigEnvironment() override;

		/**
		 * Counts the number of elements in the value at `path`, returning `0` if no value exists at
		 * the specified. Ona objects and arrays are considered "countable" types, while all other
		 * types are regarded as having a count of `1`.
		 */
		uint32_t Count(std::initializer_list<String> const & path);

		/**
		 * Parses and executes `source` as an Ona configuration file.
		 *
		 * A `ScriptError` is returned, indicating the result of parsing `source`.
		 *
		 * If `errorMessage` is not null and the returned value is not equal to `ScriptError::None`,
		 * a human-readable error message may have been written to `errorMessage`.
		 */
		ScriptError Parse(String const & source, String * errorMessage);

		/**
		 * Searches the `ConfigEnvironment` along the path node names denoted by `path`, returning
		 * the resulting boolean value at index `index`, otherwise `fallback` if the specified path
		 * does not exist.
		 *
		 * For accessing single values, specify an array `index` of `0`. Attempting to index into a
		 * non-array type at a non-zero index will fail to read the value.
		 */
		bool ReadBoolean(std::initializer_list<String> const & path, int32_t index, bool fallback);

		/**
		 * Searches the `ConfigEnvironment` along the path node names denoted by `path`, returning
		 * the resulting integer value at index `index`, otherwise `fallback` if the specified path
		 * does not exist.
		 *
		 * For accessing single values, specify an array `index` of `0`. Attempting to index into a
		 * non-array type at a non-zero index will fail to read the value.
		 */
		int64_t ReadInteger(
			std::initializer_list<String> const & path,
			int32_t index,
			int64_t fallback
		);

		/**
		 * Searches the `ConfigEnvironment` along the path node names denoted by `path`, returning
		 * the resulting floating point value at index `index`, otherwise `fallback` if the
		 * specified path does not exist.
		 *
		 * For accessing single values, specify an array `index` of `0`. Attempting to index into a
		 * non-array type at a non-zero index will fail to read the value.
		 */
		double ReadFloating(
			std::initializer_list<String> const & path,
			int32_t index,
			double fallback
		);

		/**
		 * Searches the `ConfigEnvironment` along the path node names denoted by `path`, returning
		 * the resulting `Ona::String` value at index `index`, otherwise `fallback` if the specified
		 * path does not exist.
		 *
		 * For accessing single values, specify an array `index` of `0`. Attempting to index into a
		 * non-array type at a non-zero index will fail to read the value.
		 */
		String ReadString(
			std::initializer_list<String> const & path,
			int32_t index,
			String const & fallback
		);

		/**
		 * Searches the `ConfigEnvironment` along the path node names denoted by `path`, returning
		 * the resulting `Ona::Vector2` value at index `index`, otherwise `fallback` if the
		 * specified path does not exist.
		 *
		 * For accessing single values, specify an array `index` of `0`. Attempting to index into a
		 * non-array type at a non-zero index will fail to read the value.
		 */
		Vector2 ReadVector2(
			std::initializer_list<String> const & path,
			int32_t index,
			Vector2 fallback
		);

		/**
		 * Searches the `ConfigEnvironment` along the path node names denoted by `path`, returning
		 * the resulting `Ona::Vector3` value at index `index`, otherwise `fallback` if the
		 * specified path does not exist.
		 *
		 * For accessing single values, specify an array `index` of `0`. Attempting to index into a
		 * non-array type at a non-zero index will fail to read the value.
		 */
		Vector3 ReadVector3(
			std::initializer_list<String> const & path,
			int32_t index,
			Vector3 const & fallback
		);

		/**
		 * Searches the `ConfigEnvironment` along the path node names denoted by `path`, returning
		 * the resulting `Ona::Vector4` value at index `index`, otherwise `fallback` if the
		 * specified path does not exist.
		 *
		 * For accessing single values, specify an array `index` of `0`. Attempting to index into a
		 * non-array type at a non-zero index will fail to read the value.
		 */
		Vector4 ReadVector4(
			std::initializer_list<String> const & path,
			int32_t index,
			Vector4 const & fallback
		);
	};

	#include "api.h"
}

#endif
