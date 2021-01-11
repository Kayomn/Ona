
namespace Ona::Core {
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

		/**
		 * Frees the resources associated with the `Image`.
		 */
		void Free();

		/**
		 * Attempts to create an `Image` in memory of `dimensions` pixels width and height with
		 * `pixels` as the pixel data and `allocator` as the dynamic memory allocation strategy for
		 * the `Image`.
		 *
		 * Passing a value in `pixels` value that does not point to a buffer of pixel data equal in
		 * size to the X and Y component of `dimensions is undefined behaviour.
		 *
		 * Successfully created `Images` are written to `result` and an empty `Error` is returned.
		 * Otherwise:
		 *
		 *   * `ImageError::OutOfMemory` is returned if `allocator` failed to allocate the memory
		 *     required for the `Image`.
		 *
		 *   * `ImageError::UnsupportedFormat is returned if `pixels` is `nullptr` or the components
		 *     of `dimensions` are `0` or less.
		 */
		static Error<ImageError> From(
			Allocator * allocator,
			Point2 dimensions,
			Color * pixels,
			Image & result
		);

		/**
		 * Attempts to create an `Image` in memory of `dimensions` pixels width and height with
		 * `color` as the color for all pixel data and `allocator` as the dynamic memory allocation
		 * strategy for the `Image`.
		 *
		 * Successfully created `Images` are written to `result` and an empty `Error` is returned.
		 * Otherwise:
		 *
		 *   * `ImageError::OutOfMemory` is returned if `allocator` failed to allocate the memory
		 *     required for the `Image`.
		 *
		 *   * `ImageError::UnsupportedFormat is returned if the components of `dimensions` are `0`
		 *     or less.
		 */
		static Error<ImageError> Solid(
			Allocator * allocator,
			Point2 dimensions,
			Color color,
			Image & result
		);
	};
}
