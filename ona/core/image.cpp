#include "ona/core.hpp"

namespace Ona::Core {
	void Image::Free() {
		if (this->pixels) {
			if (this->allocator) {
				this->allocator->Deallocate(reinterpret_cast<uint8_t *>(this->pixels.pointer));
			} else {
				Deallocate(reinterpret_cast<uint8_t *>(this->pixels.pointer));
			}
		}
	}

	Image Image::From(
		Allocator * allocator,
		Point2 dimensions,
		Color * pixels,
		ImageError * error
	) {
		if (pixels && (dimensions.x > 0) && (dimensions.y > 0)) {
			Image image = {allocator, dimensions};
			size_t const imageSize = static_cast<size_t>(dimensions.x * dimensions.y);

			if (allocator) {
				image.pixels = allocator->Allocate(imageSize).As<Color>();
			} else {
				image.pixels = Allocate(imageSize).As<Color>();
			}

			if ((CopyMemory(
				image.pixels.AsBytes(),
				SliceOf(pixels, imageSize).AsBytes()
			) != imageSize) && error) (*error) = ImageError::OutOfMemory;

			return image;
		} else if (error) (*error) = ImageError::UnsupportedFormat;

		return Image{};
	}

	Image Image::Solid(
		Allocator * allocator,
		Point2 const & dimensions,
		Color color,
		ImageError * error
	) {
		if ((dimensions.x > 0) && (dimensions.y > 0)) {
			Slice<Color> pixels = (
				allocator ?
				allocator->Allocate(static_cast<size_t>(dimensions.x * dimensions.y)).As<Color>() :
				Allocate(static_cast<size_t>(dimensions.x * dimensions.y)).As<Color>()
			);

			if (pixels.length) {
				WriteMemory(pixels, color);

				return Image{allocator, dimensions, pixels};
			} else if (error) (*error) = ImageError::OutOfMemory;
		} else if (error) (*error) = ImageError::UnsupportedFormat;

		return Image{};
	}
}
