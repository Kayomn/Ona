#include "ona/core.hpp"

using ImageResult = Ona::Core::Result<Ona::Core::Image, Ona::Core::ImageError>;

namespace Ona::Core {
	void Image::Free() {
		if (this->pixels.HasValue()) {
			if (this->allocator.HasValue()) {
				this->allocator.Value()->Deallocate(
					reinterpret_cast<uint8_t *>(this->pixels.pointer)
				);
			} else {
				Deallocate(reinterpret_cast<uint8_t *>(this->pixels.pointer));
			}
		}
	}

	Result<Image, ImageError> Image::From(
		Optional<Allocator *> allocator,
		Point2 dimensions,
		Color * pixels
	) {
		if (pixels && (dimensions.x > 0) && (dimensions.y > 0)) {
			Image image = {allocator, dimensions};
			size_t const imageSize = static_cast<size_t>(dimensions.x * dimensions.y);

			image.pixels = (
				allocator.HasValue() ?
				allocator.Value()->Allocate(imageSize * sizeof(Color)).As<Color>() :
				Allocate(imageSize * sizeof(Color)).As<Color>()
			);

			if (image.pixels.HasValue()) {
				CopyMemory(image.pixels.AsBytes(), SliceOf(pixels, imageSize).AsBytes());

				return ImageResult::Ok(image);
			}

			return ImageResult::Fail(ImageError::OutOfMemory);
		}

		return ImageResult::Fail(ImageError::UnsupportedFormat);
	}

	Result<Image, ImageError> Image::Solid(
		Optional<Allocator *> allocator,
		Point2 dimensions,
		Color color
	) {
		if ((dimensions.x > 0) && (dimensions.y > 0)) {
			Slice<Color> pixels = (
				allocator.HasValue() ?

				allocator.Value()->Allocate(
					static_cast<size_t>(dimensions.x * dimensions.y) *
					sizeof(Color)
				).As<Color>() :

				Allocate(
					static_cast<size_t>(dimensions.x * dimensions.y) *
					sizeof(Color)
				).As<Color>()
			);

			if (pixels.length) {
				WriteMemory(pixels, color);

				return ImageResult::Ok(Image{
					allocator,
					dimensions,
					pixels
				});
			}

			return ImageResult::Fail(ImageError::OutOfMemory);
		}

		return ImageResult::Fail(ImageError::UnsupportedFormat);
	}
}
