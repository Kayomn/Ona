#include "engine.hpp"

namespace Ona {
	static HashTable<String, ImageLoader> imageLoaders = {Allocator::Default};

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
			Deallocate(this->allocator, this->pixels);

			this->pixels = nullptr;
			this->dimensions = Point2{};
		}
	}

	bool Image::From(Allocator allocator, Point2 dimensions, Color * pixels, Image * result) {
		if (pixels && (dimensions.x > 0) && (dimensions.y > 0)) {
			DynamicArray<uint8_t> pixelBuffer = {allocator, (Area(dimensions) * sizeof(Color))};

			if (pixelBuffer.Length()) {
				CopyMemory(pixelBuffer.Values(), Slice<uint8_t>{
					.length = pixelBuffer.Length(),
					.pointer = reinterpret_cast<uint8_t *>(pixels),
				});

				(*result) = Image{
					.allocator = allocator,
					.pixels = reinterpret_cast<Color* >(pixelBuffer.Release().pointer),
					.dimensions = dimensions,
				};

				return true;
			}
		}

		return false;
	}

	bool Image::Solid(Allocator allocator, Point2 dimensions, Color color, Image * result) {
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

				(*result) = Image{
					.allocator = allocator,
					.pixels = reinterpret_cast<Color *>(pixelBuffer.Release().pointer),
					.dimensions = dimensions,
				};

				return true;
			}
		}

		return false;
	}

	void RegisterImageLoader(String const & fileFormat, ImageLoader imageLoader) {
		imageLoaders.Insert(fileFormat, imageLoader);
	}

	bool LoadImage(Stream * stream, Allocator allocator, Image * imageResult) {
		ImageLoader * imageLoader = imageLoaders.Lookup(PathExtension(stream->ID()));

		if (imageLoader) {
			return (*imageLoader)(stream, allocator, imageResult);
		}

		return false;
	}
}
