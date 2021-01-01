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

	Result<Image, ImageError> LoadBitmap(Allocator * imageAllocator, String const & fileName) {
		using Res = Result<Image, ImageError>;

		struct $packed FileHeader {
			uint16_t signature;

			uint32_t fileSize;

			uint32_t filePadding;

			uint32_t fileOffset;
		};

		struct $packed InfoHeader {
			uint32_t headerSize;

			uint32_t imageWidth;

			uint32_t imageHeight;

			uint16_t imagePlanes;

			uint16_t bitCount;

			uint32_t compression;

			uint32_t sizeImage;

			uint32_t pixelsPerMeterX;

			uint32_t pixelsPerMeterY;

			uint32_t colorsUsed;

			uint32_t colorsImportant;
		};

		enum {
			Signature = 0x4d42,
			FileHeaderSize = 14,
			InfoHeaderMinSize = 40,
		};

		enum {
			CompressionRGB = 0x00,
			CompressionRLE8 = 0x01, // compressed
			CompressionRLE4 = 0x02, // compressed
			CompressionBITFIELDS = 0x03,
			CompressionJPEG = 0x04,
			CompressionPNG = 0x05,
			CompressionALPHABITFIELDS = 0x06,
			CompressionCMYK = 0x0b,
			CompressionCMYKRLE8 = 0x0c, // compressed
			CompressionCMYKRLE4 = 0x0d // compressed
		};

		static auto isCompressedFormat = [](uint32_t compression) -> bool {
			switch (compression) {
				case CompressionRLE8:
				case CompressionRLE4:
				case CompressionCMYKRLE8:
				case CompressionCMYKRLE4: return true;
			}

			return false;
		};

		Owned<File> file = OpenFile(fileName, File::OpenRead);

		if (file.value.IsOpen()) {
			InlineArray<uint8_t, (sizeof(FileHeader) + sizeof(InfoHeader))> headerBuffer = {};
			uint8_t const * const headerPointer = headerBuffer.Pointer();
			auto const fileHeader = reinterpret_cast<FileHeader const *>(headerPointer);

			if (
				(file.value.Read(headerBuffer.Sliced(0, FileHeaderSize)) == FileHeaderSize) &&
				(fileHeader->signature == Signature) &&
				(file.value.Read(headerBuffer.Sliced(sizeof(FileHeader), InfoHeaderMinSize)) == InfoHeaderMinSize)
			) {
				InfoHeader const * const infoHeader = reinterpret_cast<InfoHeader const *>(
					(headerPointer + sizeof(FileHeader))
				);

				if (
					(infoHeader->headerSize >= InfoHeaderMinSize) &&
					(!isCompressedFormat(infoHeader->compression)) &&
					(infoHeader->imageWidth < INT32_MAX) &&
					(infoHeader->imageHeight < INT32_MAX)
				) {
					Point2 const dimensions = {
						.x = static_cast<int32_t>(infoHeader->imageWidth),
						.y = static_cast<int32_t>(infoHeader->imageHeight)
					};

					int64_t const imageSize = Area(dimensions);

					if (imageSize < SIZE_MAX) {
						Slice<uint8_t> pixelBuffer = imageAllocator->Allocate(
							imageSize * sizeof(Color)
						);

						if (pixelBuffer.length) switch (infoHeader->bitCount) {
							case 24: {
								enum {
									BytesPerPixel = 3,
								};

								uint64_t const rowWidth = (dimensions.x * 3);

								// Row requires additional padding, hence the magic numbers.
								Slice<uint8_t> rowBuffer = imageAllocator->Allocate(
									(rowWidth + BytesPerPixel) & (~BytesPerPixel)
								);

								if (rowBuffer.length) {
									size_t pixelIndex = (pixelBuffer.length - 1);

									file.value.SeekHead(fileHeader->fileOffset);

									for (uint32_t i = 0; i < dimensions.y; i += 1) {
										file.value.Read(rowBuffer);

										for (uint32_t j = 0; j < rowWidth; j += BytesPerPixel) {
											// Swap around BGR -> RGB and write alpha channel.
											pixelBuffer.At(pixelIndex - 3) = rowBuffer.At(j + 2);
											pixelBuffer.At(pixelIndex - 2) = rowBuffer.At(j + 1);
											pixelBuffer.At(pixelIndex - 1) = rowBuffer.At(j);
											pixelBuffer.At(pixelIndex) = 0xFF;
											pixelIndex -= sizeof(Color);
										}
									}

									return Res::Ok(Image{
										.allocator = imageAllocator,
										.pixels = reinterpret_cast<Color *>(pixelBuffer.pointer),
										.dimensions = dimensions,
									});
								}

								imageAllocator->Deallocate(pixelBuffer.pointer);

								// Unable to allocate row buffer.
								return Res::Fail(ImageError::OutOfMemory);
							} break;

							case 32: {
								enum {
									BytesPerPixel = 4,
								};

								uint64_t const rowWidth = (dimensions.x * BytesPerPixel);
								Slice<uint8_t> rowBuffer = imageAllocator->Allocate(rowWidth);

								if (rowBuffer.length) {
									size_t pixelIndex = (pixelBuffer.length - 1);

									file.value.SeekHead(fileHeader->fileOffset);

									for (uint32_t i = 0; i < dimensions.y; i += 1) {
										file.value.Read(rowBuffer);

										for (uint32_t j = 0; j < rowWidth; j += BytesPerPixel) {
											// Swap around BGRA -> RGBA.
											pixelBuffer.At(pixelIndex - 3) = rowBuffer.At(j + 2);
											pixelBuffer.At(pixelIndex - 2) = rowBuffer.At(j + 1);
											pixelBuffer.At(pixelIndex - 1) = rowBuffer.At(j);
											pixelBuffer.At(pixelIndex) = rowBuffer.At(j + 3);
											pixelIndex -= sizeof(Color);
										}
									}

									return Res::Ok(Image{
										.allocator = imageAllocator,
										.pixels = reinterpret_cast<Color *>(pixelBuffer.pointer),
										.dimensions = dimensions,
									});
								}

								imageAllocator->Deallocate(pixelBuffer.pointer);

								// Unable to allocate row buffer.
								return Res::Fail(ImageError::OutOfMemory);
							} break;

							// Unsupporterd bit per pixel format.
							default: return Res::Fail(ImageError::UnsupportedFormat);
						}

						// Unable to allocate pixel buffer.
						return Res::Fail(ImageError::OutOfMemory);
					}
				}

				// Invalid info header, image too big, or bitmap format is compressed.
				return Res::Fail(ImageError::UnsupportedFormat);
			}

			// Invalid file or info header.
			return Res::Fail(ImageError::UnsupportedFormat);
		}

		return Res::Fail(ImageError::IO);
	}
}
