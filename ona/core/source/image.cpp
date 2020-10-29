#include "ona/core/module.hpp"

namespace Ona::Core {
	void Image::Free() {
		if (this->pixels.length) {
			this->allocator->Deallocate(this->pixels.pointer);
		}
	}

	Result<Image, ImageError> Image::From(
		Allocator * allocator,
		Point2 dimensions,
		Color * pixels
	) {
		using Res = Result<Image, ImageError>;

		if (pixels && (dimensions.x > 0) && (dimensions.y > 0)) {
			Image image = Image{
				.allocator = allocator,
				.dimensions = dimensions
			};

			size_t const imageSize = static_cast<size_t>(dimensions.x * dimensions.y);
			image.pixels = allocator->Allocate(imageSize * sizeof(Color)).As<Color>();

			if (image.pixels.length) {
				CopyMemory(image.pixels.AsBytes(), SliceOf(pixels, imageSize).AsBytes());

				return Res::Ok(image);
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

			Slice<uint8_t> pixels = allocator->Allocate(pixelArea * sizeof(Color));

			if (pixels.length) {
				for (size_t i = 0; i < pixelArea; i += 1) {
					CopyMemory(pixels.Sliced((i * sizeof(Color)), pixels.length), AsBytes(color));
				}

				return Res::Ok(Image{
					allocator,
					dimensions,
					pixels.As<Color>()
				});
			}

			return Res::Fail(ImageError::OutOfMemory);
		}

		return Res::Fail(ImageError::UnsupportedFormat);
	}
}
