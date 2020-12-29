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

	Result<Image, ImageError> Image::From(
		Allocator * allocator,
		Point2 dimensions,
		Color * pixels
	) {
		using Res = Result<Image, ImageError>;

		if (pixels && (dimensions.x > 0) && (dimensions.y > 0)) {
			size_t const imageSize = static_cast<size_t>(dimensions.x * dimensions.y);
			Slice<uint8_t> pixelBuffer = allocator->Allocate(imageSize * sizeof(Color));

			if (pixelBuffer.length) {
				CopyMemory(pixelBuffer, SliceOf(pixels, imageSize).AsBytes());

				return Res::Ok(Image{
					.allocator = allocator,
					.pixels = reinterpret_cast<Color* >(pixelBuffer.pointer),
					.dimensions = dimensions,
				});
			}

			return Res::Fail(ImageError::OutOfMemory);
		}

		return Res::Fail(ImageError::UnsupportedFormat);
	}

	Result<Image, ImageError> Image::Solid(
		Allocator * allocator,
		Point2 dimensions,
		Color color
	) {
		using Res = Result<Image, ImageError>;

		if ((dimensions.x > 0) && (dimensions.y > 0)) {
			size_t const pixelArea = static_cast<size_t>(dimensions.x * dimensions.y);
			Slice<uint8_t> pixelBuffer = allocator->Allocate(pixelArea * sizeof(Color));

			if (pixelBuffer.length) {
				for (size_t i = 0; i < pixelArea; i += 1) {
					CopyMemory(
						pixelBuffer.Sliced((i * sizeof(Color)), pixelBuffer.length),
						AsBytes(color)
					);
				}

				return Res::Ok(Image{
					.allocator = allocator,
					.pixels = reinterpret_cast<Color *>(pixelBuffer.pointer),
					.dimensions = dimensions,
				});
			}

			return Res::Fail(ImageError::OutOfMemory);
		}

		return Res::Fail(ImageError::UnsupportedFormat);
	}
}
