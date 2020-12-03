
namespace Ona::Core {
	struct Color {
		uint8_t r, g, b, a;
	};

	constexpr Color Greyscale(uint8_t const value) {
		return Color{value, value, value, 0xFF};
	}

	constexpr Color RGB(uint8_t const red, uint8_t const green, uint8_t const blue) {
		return Color{red, green, blue, 0xFF};
	}

	enum class ImageError {
		None,
		UnsupportedFormat,
		OutOfMemory
	};

	struct Image {
		Allocator * allocator;

		Color * pixels;

		Point2 dimensions;

		void Free();

		static Result<Image, ImageError> From(
			Allocator * allocator,
			Point2 dimensions,
			Color * pixels
		);

		static Result<Image, ImageError> Solid(
			Allocator * allocator,
			Point2 dimensions,
			Color color
		);
	};
}
