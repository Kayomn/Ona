#include "ona/engine/module.hpp"

namespace Ona::Engine {
	using namespace Ona::Core;

	String LoadText(Allocator * tempAllocator, FileServer * fileServer, String const & filePath) {
		File file;

		if (fileServer->OpenFile(filePath, file, File::OpenRead)) {
			int64_t const fileSize = fileServer->SeekTail(file, 0);

			fileServer->SeekHead(file, 0);

			DynamicArray<char> fileBuffer = {tempAllocator, static_cast<size_t>(fileSize)};

			if (fileBuffer.Length()) {
				fileServer->Read(file, fileBuffer.Values().Bytes());
				fileServer->CloseFile(file);

				return String{fileBuffer.Values()};
			}

			fileServer->CloseFile(file);
		}

		return String{};
	}

	Result<Image, ImageError> LoadBitmap(
		Allocator * imageAllocator,
		FileServer * fileServer,
		String const & filePath
	) {
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

		File file;

		if (fileServer->OpenFile(filePath, file, File::OpenRead)) {
			InlineArray<uint8_t, (sizeof(FileHeader) + sizeof(InfoHeader))> headerBuffer = {};
			uint8_t const * const headerPointer = headerBuffer.Pointer();
			auto const fileHeader = reinterpret_cast<FileHeader const *>(headerPointer);

			if (
				(fileServer->Read(file, headerBuffer.Sliced(0, FileHeaderSize)) == FileHeaderSize) &&
				(fileHeader->signature == Signature) &&
				(fileServer->Read(file, headerBuffer.Sliced(sizeof(FileHeader), InfoHeaderMinSize)) == InfoHeaderMinSize)
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
						DynamicArray<uint8_t> pixelBuffer = {
							imageAllocator,
							(imageSize * sizeof(Color))
						};

						size_t const pixelBufferSize = pixelBuffer.Length();

						// Bitmaps are formatted as BGRA and are also upside down. As I understand
						// it, this is due to some early decisions made by mathematicians at IBM
						// that Y increases as it ascends. This wouldn't be as bad if horizontal
						// coordinates were also inverted, but they're not, so the file has to be
						// read left-to-right, bottom-to-top.
						if (pixelBufferSize) switch (infoHeader->bitCount) {
							case 24: {
								enum {
									BytesPerPixel = 3,
								};

								uint64_t const rowWidth = (dimensions.x * 3);

								// Row requires additional padding, hence the magic numbers.
								DynamicArray<uint8_t> rowBuffer = {
									imageAllocator,
									(rowWidth + BytesPerPixel) & (~BytesPerPixel)
								};

								if (rowBuffer.Length()) {
									size_t pixelIndex = (pixelBufferSize - 1);

									fileServer->SeekHead(file, fileHeader->fileOffset);

									for (uint32_t i = 0; i < dimensions.y; i += 1) {
										size_t destinationIndex =
											(pixelBufferSize - (rowBuffer.Length() * (i + 1)));

										size_t sourceIndex = 0;

										fileServer->Read(file, rowBuffer.Values());

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

									fileServer->CloseFile(file);

									return Res::Ok(Image{
										.allocator = imageAllocator,

										.pixels = reinterpret_cast<Color *>(
											pixelBuffer.Release().pointer
										),

										.dimensions = dimensions,
									});
								}

								// Unable to allocate row buffer.
								return Res::Fail(ImageError::OutOfMemory);
							} break;

							case 32: {
								enum {
									BytesPerPixel = 4,
								};

								uint64_t const rowWidth = (dimensions.x * BytesPerPixel);
								DynamicArray<uint8_t> rowBuffer = {imageAllocator, rowWidth};

								if (rowBuffer.Length()) {
									fileServer->SeekHead(file, fileHeader->fileOffset);

									for (size_t i = 0; i < dimensions.y; i += 1) {
										size_t destinationIndex =
											(pixelBufferSize - (rowBuffer.Length() * (i + 1)));

										size_t sourceIndex = 0;

										fileServer->Read(file, rowBuffer.Values());

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

									fileServer->CloseFile(file);

									return Res::Ok(Image{
										.allocator = imageAllocator,

										.pixels = reinterpret_cast<Color *>(
											pixelBuffer.Release().pointer
										),

										.dimensions = dimensions,
									});
								}

								fileServer->CloseFile(file);

								// Unable to allocate row buffer.
								return Res::Fail(ImageError::OutOfMemory);
							} break;

							// Unsupporterd bit per pixel format.
							default: {
								fileServer->CloseFile(file);

								return Res::Fail(ImageError::UnsupportedFormat);
							}
						}

						fileServer->CloseFile(file);

						// Unable to allocate pixel buffer.
						return Res::Fail(ImageError::OutOfMemory);
					}
				}

				fileServer->CloseFile(file);

				// Invalid info header, image too big, or bitmap format is compressed.
				return Res::Fail(ImageError::UnsupportedFormat);
			}

			fileServer->CloseFile(file);

			// Invalid file or info header.
			return Res::Fail(ImageError::UnsupportedFormat);
		}

		// I/O operation failed, load fallback image instead.
		return Image::Solid(imageAllocator, Point2{32, 32}, RGB(0xFF, 0, 0xFF));
	}
}
