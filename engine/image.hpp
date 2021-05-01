
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
	constexpr Color Greyscale(float const value) {
		return Color{
			.r = static_cast<uint8_t>(UINT8_MAX * value),
			.g = static_cast<uint8_t>(UINT8_MAX * value),
			.b = static_cast<uint8_t>(UINT8_MAX * value),
			.a = 0xFF
		};
	}

	/**
	 * Creates an opaque RGB `Color` from `red`, `green`, and `blue.
	 */
	constexpr Color Rgb(float const red, float const green, float const blue) {
		return Color{
			.r = static_cast<uint8_t>(UINT8_MAX * red),
			.g = static_cast<uint8_t>(UINT8_MAX * green),
			.b = static_cast<uint8_t>(UINT8_MAX * blue),
			.a = UINT8_MAX
		};
	}

	/**
	 * Resource handle to a buffer of 32-bit RGBA pixel data.
	 */
	class Image : public Object {
		private:
		Allocator allocator;

		Color * pixels;

		Point2 dimensions;

		public:
		Image() = default;

		~Image() override;

		/**
		 * Returns the dimensions of the image with the width and height mapped to the `x` and `y`
		 * components of `Ona::Point2` respectively.
		 */
		Point2 Dimensions() const;

		/**
		 * Attempts to load image data from `stream`, as identified by the path extension on
		 * `SystemStream::ID`, into `result` using `allocator`.
		 */
		bool LoadBitmap(Stream * stream, Allocator allocator);

		/**
		 * Attempts to load a solid image of size `dimensions` with `fillColor` as the image color
		 * using `allocator`.
		 */
		bool LoadSolid(Color fillColor, Point2 dimensions, Allocator allocator);

		/**
		 * Returns a linear one-dimension view of the image pixels.
		 */
		Slice<Color> Pixels() const;
	};
}
