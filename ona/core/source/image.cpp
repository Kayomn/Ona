#include "ona/core/module.hpp"

namespace Ona::Core {
	Vector4 Color::Normalized() const {
		return Vector4{
			(this->r / static_cast<float>(0xFF)),
			(this->g / static_cast<float>(0xFF)),
			(this->b / static_cast<float>(0xFF)),
			(this->a / static_cast<float>(0xFF))
		};
	}

	void Image::Free() {
		if (this->pixels) {
			this->allocator->Deallocate(this->pixels);

			this->pixels = nullptr;
			this->dimensions = Point2{};
		}
	}

	Error<ImageError> Image::From(
		Allocator * allocator,
		Point2 dimensions,
		Color * pixels,
		Image & result
	) {
		using Err = Error<ImageError>;

		if (pixels && (dimensions.x > 0) && (dimensions.y > 0)) {
			DynamicArray<uint8_t> pixelBuffer = {allocator, (Area(dimensions) * sizeof(Color))};

			if (pixelBuffer.Length()) {
				CopyMemory(pixelBuffer.Values(), Slice<uint8_t>{
					.length = pixelBuffer.Length(),
					.pointer = reinterpret_cast<uint8_t *>(pixels),
				});

				result = Image{
					.allocator = allocator,
					.pixels = reinterpret_cast<Color* >(pixelBuffer.Release().pointer),
					.dimensions = dimensions,
				};
			}

			return Err{ImageError::OutOfMemory};
		}

		return Err{ImageError::UnsupportedFormat};
	}

	Error<ImageError> Image::Solid(
		Allocator * allocator,
		Point2 dimensions,
		Color color,
		Image & result
	) {
		using Err = Error<ImageError>;

		if ((dimensions.x > 0) && (dimensions.y > 0)) {
			int64_t const pixelArea = Area(dimensions);
			DynamicArray<uint8_t> pixelBuffer = {allocator, (pixelArea * sizeof(Color))};
			size_t const pixelBufferSize = pixelBuffer.Length();

			if (pixelBufferSize) {
				for (size_t i = 0; i < pixelArea; i += 1) {
					CopyMemory(
						pixelBuffer.Sliced((i * sizeof(Color)), pixelBufferSize),
						AsBytes(color)
					);
				}

				result = Image{
					.allocator = allocator,
					.pixels = reinterpret_cast<Color *>(pixelBuffer.Release().pointer),
					.dimensions = dimensions,
				};

				return Err{};
			}

			return Err{ImageError::OutOfMemory};
		}

		return Err{ImageError::UnsupportedFormat};
	}
}
