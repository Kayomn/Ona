#include "engine.hpp"

namespace Ona {
	Vector4 Color::Normalized() const {
		return Vector4{
			(this->r / static_cast<float>(0xFF)),
			(this->g / static_cast<float>(0xFF)),
			(this->b / static_cast<float>(0xFF)),
			(this->a / static_cast<float>(0xFF))
		};
	}

	Image::~Image() {
		if (this->pixels) {
			Deallocate(this->allocator, this->pixels);

			this->pixels = nullptr;
			this->dimensions = Point2{};
		}
	}

	Point2 Image::Dimensions() const {
		return this->dimensions;
	}

	bool Image::LoadBitmap(Stream * stream, Allocator allocator) {
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

		FixedArray<uint8_t, (sizeof(FileHeader) + sizeof(InfoHeader))> headerBuffer = {};
		uint8_t const * const headerPointer = headerBuffer.Pointer();
		auto const fileHeader = reinterpret_cast<FileHeader const *>(headerPointer);

		if (
			(stream->ReadBytes(headerBuffer.Sliced(0, FileHeaderSize)) == FileHeaderSize) &&
			(fileHeader->signature == Signature) &&
			(stream->ReadBytes(headerBuffer.Sliced(sizeof(FileHeader), InfoHeaderMinSize)) == InfoHeaderMinSize)
		) {
			InfoHeader const * const infoHeader =
					reinterpret_cast<InfoHeader const *>(headerPointer + sizeof(FileHeader));

			if (
				(infoHeader->headerSize >= InfoHeaderMinSize) &&
				(!isCompressedFormat(infoHeader->compression)) &&
				(infoHeader->imageWidth < INT32_MAX) &&
				(infoHeader->imageHeight < INT32_MAX)
			) {
				this->dimensions = Point2{
					.x = static_cast<int32_t>(infoHeader->imageWidth),
					.y = static_cast<int32_t>(infoHeader->imageHeight)
				};

				int64_t const imageSize = Area(dimensions);

				if (imageSize < SIZE_MAX) {
					DynamicArray<uint8_t> pixelBuffer = {allocator, (imageSize * sizeof(Color))};
					size_t const pixelBufferSize = pixelBuffer.Length();

					// Bitmaps are formatted as BGRA and are also upside down. As I understand it,
					// this is due to some early decisions made by mathematicians at IBM that Y
					// increases as it ascends. This wouldn't be as bad if horizontal coordinates
					// were also inverted, but they're not, so the file has to be read left-to-
					// right, bottom-to-top.
					if (pixelBufferSize) switch (infoHeader->bitCount) {
						case 24: {
							enum {
								BytesPerPixel = 3,
							};

							uint64_t const rowWidth = (dimensions.x * 3);

							// Row requires additional padding, hence the magic numbers.
							DynamicArray<uint8_t> rowBuffer = {
								allocator,
								(rowWidth + BytesPerPixel) & (~BytesPerPixel)
							};

							if (rowBuffer.Length()) {
								size_t pixelIndex = (pixelBufferSize - 1);

								stream->SeekHead(fileHeader->fileOffset);

								for (uint32_t i = 0; i < dimensions.y; i += 1) {
									size_t destinationIndex =
										(pixelBufferSize - (rowBuffer.Length() * (i + 1)));

									size_t sourceIndex = 0;

									if (stream->ReadBytes(
										rowBuffer.Values()
									) != rowBuffer.Length()) {
										// Could not read image row.
										return false;
									}

									while (sourceIndex < rowBuffer.Length()) {
										// Swap around BGR -> RGB and write alpha channel.
										pixelBuffer.At(destinationIndex) =
											rowBuffer.At(sourceIndex + 2);

										pixelBuffer.At(destinationIndex + 1) =
											rowBuffer.At(sourceIndex + 1);

										pixelBuffer.At(destinationIndex + 2) =
											rowBuffer.At(sourceIndex);

										pixelBuffer.At(destinationIndex + 3) = 0xFF;
										sourceIndex += BytesPerPixel;
										destinationIndex += BytesPerPixel;
									}
								}

								this->allocator = allocator;

								this->pixels =
										reinterpret_cast<Color *>(pixelBuffer.Release().pointer);

								return true;
							}

							// Unable to allocate row buffer.
						} break;

						case 32: {
							enum {
								BytesPerPixel = 4,
							};

							uint64_t const rowWidth = (dimensions.x * BytesPerPixel);
							DynamicArray<uint8_t> rowBuffer = {allocator, rowWidth};

							if (rowBuffer.Length()) {
								stream->SeekHead(fileHeader->fileOffset);

								for (size_t i = 0; i < dimensions.y; i += 1) {
									size_t destinationIndex =
										(pixelBufferSize - (rowBuffer.Length() * (i + 1)));

									size_t sourceIndex = 0;

									if (stream->ReadBytes(
										rowBuffer.Values()
									) != rowBuffer.Length()) {
										// Could not read image row.
										return false;
									}

									while (sourceIndex < rowBuffer.Length()) {
										// Swap around BGRA -> RGBA.
										pixelBuffer.At(destinationIndex) =
											rowBuffer.At(sourceIndex + 2);

										pixelBuffer.At(destinationIndex + 1) =
											rowBuffer.At(sourceIndex + 1);

										pixelBuffer.At(destinationIndex + 2) =
											rowBuffer.At(sourceIndex);

										pixelBuffer.At(destinationIndex + 3) =
											rowBuffer.At(sourceIndex + 3);

										sourceIndex += BytesPerPixel;
										destinationIndex += BytesPerPixel;
									}
								}

								this->allocator = allocator;

								this->pixels =
										reinterpret_cast<Color *>(pixelBuffer.Release().pointer);

								return true;
							}

							// Unable to allocate row buffer.
							return false;
						}

						// Unsupporterd bit per pixel format.
						default: break;
					}
					// Unable to allocate pixel buffer.
				}
			}
			// Invalid info header, image too big, or bitmap format is compressed.
		}

		// Invalid file or info header.
		return false;
	}

	bool Image::LoadSolid(Color fillColor, Point2 dimensions, Allocator allocator) {
		if ((dimensions.x > 0) && (dimensions.y > 0)) {
			int64_t const pixelArea = Area(dimensions);
			DynamicArray<uint8_t> pixelBuffer = {allocator, (pixelArea * sizeof(Color))};
			size_t const pixelBufferSize = pixelBuffer.Length();

			if (pixelBufferSize) {
				for (size_t i = 0; i < pixelArea; i += 1) {
					CopyMemory(
						pixelBuffer.Sliced((i * sizeof(Color)), pixelBufferSize),
						AsBytes(fillColor)
					);
				}

				this->allocator = allocator;
				this->pixels = reinterpret_cast<Color *>(pixelBuffer.Release().pointer);
				this->dimensions = dimensions;

				return true;
			}
		}

		return false;
	}

	Slice<Color> Image::Pixels() const {
		return Slice<Color>{
			.length = static_cast<size_t>(Area(this->dimensions)),
			.pointer = this->pixels
		};
	}
}
