/**
 * CPU-bound image data manipulation utilies.
 */
module ona.image;

import core.memory, ona.functional, ona.math, ona.stream;

/**
 * 32-bit 16,581,375 color encoding with alpha channel.
 */
public struct Color {
	enum {
		/**
		 * Absolute black constant.
		 */
		black = Color(0, 0, 0),

		/**
		 * Absolute blue constant.
		 */
		blue = Color(0, 0, 255),

		/**
		 * Absolute green constant.
		 */
		green = Color(0, 255, 0),

		/**
		 * Absolute red constant.
		 */
		red = Color(255, 0, 0),

		/**
		 * Absolute white constant.
		 */
		white = Color(255, 255, 255),
	}

	/**
	 * Red color channel.
	 */
	ubyte r;

	/**
	 * Green color channel.
	 */
	ubyte g;

	/**
	 * Blue color channel.
	 */
	ubyte b;

	/**
	 * Transparency color channel.
	 */
	ubyte a;

	/**
	 * Assigns `red`, `green`, and `blue` as the red, green, and blue color values with an opaque
	 * alpha.
	 */
	this(in ubyte red, in ubyte green, in ubyte blue) pure {
		this(red, green, blue, ubyte.max);
	}

	/**
	 * Assigns `red`, `green`, `blue`, and `alpha` as the red, green, blue, and alpha values
	 * respectively.
	 */
	this(in ubyte red, in ubyte green, in ubyte blue, in ubyte alpha) pure {
		this.r = red;
		this.g = green;
		this.b = blue;
		this.a = alpha;
	}

	/**
	 * Returns a darkened version of the r, g, b color values.
	 */
	Color darkened(in float value) const pure {
		return Color(
			cast(ubyte)((cast(float)this.r) - (this.r * value)),
			cast(ubyte)((cast(float)this.g) - (this.g * value)),
			cast(ubyte)((cast(float)this.b) - (this.b * value)),
			this.a
		);
	}

	/**
	 * Returns a lightened version of the r, g, b color values.
	 */
	Color lightened(in float value) const pure {
		return Color(
			cast(ubyte)(this.r * value),
			cast(ubyte)(this.g * value),
			cast(ubyte)(this.b * value),
			this.a
		);
	}

	/**
	 * Returns the red, green, blue, and alpha channels as a `Vector4` of values ranging between `0`
	 * and `1`.
	 */
	Vector4 normalized() const pure {
		immutable channelMax = (cast(float)ubyte.max);

		return Vector4(
			this.r / channelMax,
			this.g / channelMax,
			this.b / channelMax,
			this.a / channelMax
		);
	}

	/**
	 * Returns the raw, unique identity for the currently contained color value.
	 */
	uint value() const pure {
		return (*cast(uint*)&this);
	}
}

/**
 * CPU-bound 2D view of pixel data depicted as individual [Color] values.
 */
public final class Image {
	private Color* pixelBuffer = null;

	private Point2 pixelDimensions;

	private this(in Point2 dimensions) {
		this.pixelBuffer = (cast(Color*)pureMalloc(Color.sizeof * dimensions.x * dimensions.y));

		assert(this.pixelBuffer, typeof(this).stringof ~ " construction out of memory");

		this.pixelDimensions = dimensions;
	}

	/**
	 * Returns the x and y pixel dimensions of the image data.
	 */
	public Point2 dimensions() const {
		return this.pixelDimensions;
	}

	/**
	 * Attempts to load a bitmap from `data`, returning it or nothing wrapped in an [Optional].
	 */
	public static Optional!Image loadBmp(ubyte[] data) {
		enum Compression {
			rgb = 0x00,
			rle8 = 0x01, // compressed
			rle4 = 0x02, // compressed
			bitfields = 0x03,
			jpeg = 0x04,
			png = 0x05,
			alphaBitfields = 0x06,
			cmyk = 0x0b,
			cmykrle8 = 0x0c, // compressed
			cmykrle4 = 0x0d, // compressed
		}

		enum signature = 0x4d42;
		enum fileHeaderSize = 14;
		enum infoHeaderMinSize = 40;

		struct FileHeader {
			align (1) ushort signature;

			align (1) uint fileSize;

			align (1) uint filePadding;

			align (1) uint fileOffset;
		}

		struct InfoHeader {
			align (1) uint headerSize;

			align (1) uint imageWidth;

			align (1) uint imageHeight;

			align (1) ushort imagePlanes;

			align (1) ushort bitCount;

			align (1) uint compression;

			align (1) uint sizeImage;

			align (1) uint pixelsPerMeterX;

			align (1) uint pixelsPerMeterY;

			align (1) uint colorsUsed;

			align (1) uint colorsImportant;
		}

		// TODO: Tidy up function.

		static bool isCompressedFormat(in uint compression) {
			switch (compression) {
				case Compression.rle8, Compression.rle4,
					Compression.cmykrle8, Compression.cmykrle4: return true;

				default: return false;
			}
		}

		scope stream = new MemoryStream(data);
		ubyte[FileHeader.sizeof + InfoHeader.sizeof] headerBuffer = 0;

		if (stream.readBytes(headerBuffer[0 .. fileHeaderSize]) != fileHeaderSize) {
			return Optional!Image();
		}

		const fileHeader = (cast(const (FileHeader*))headerBuffer.ptr);

		if (fileHeader.signature == signature) {
			if (stream.readBytes(
				headerBuffer[FileHeader.sizeof .. (FileHeader.sizeof + infoHeaderMinSize)]
			) < infoHeaderMinSize) {
				return Optional!Image();
			}

			const infoHeader = (cast(const (InfoHeader*))(headerBuffer.ptr + FileHeader.sizeof));

			if (
				(infoHeader.headerSize >= infoHeaderMinSize) &&
				(!isCompressedFormat(infoHeader.compression)) &&
				(infoHeader.imageWidth < int.max) &&
				(infoHeader.imageHeight < int.max)
			) {
				immutable dimensions = Point2(
					cast(int)infoHeader.imageWidth,
					cast(int)infoHeader.imageHeight
				);

				auto image = new Image(dimensions);
				auto pixelBuffer = (cast(ubyte[])image[]);

				// Bitmaps are formatted as BGRA and are also upside down. As I understand it, this
				// is due to some early decisions made by mathematicians at IBM that Y increases as
				// it ascends. This wouldn't be as bad if horizontal coordinates were also inverted,
				// but they're not, so the file has to be read left-to-right, bottom-to-top.
				pixelEncodingCheck: switch (infoHeader.bitCount) {
					case 24: {
						enum bytesPerPixel = 3;
						immutable (size_t) rowWidth = (dimensions.x * 3);
						// Bitmap rows require additional padding, hence the magic bitshifting.
						auto rowBuffer = new ubyte[(rowWidth + bytesPerPixel) & (~bytesPerPixel)];

						stream.seekHead(fileHeader.fileOffset);

						foreach (i; 0 .. dimensions.y) {
							size_t destinationIndex =
								(pixelBuffer.length - (rowBuffer.length * (i + 1)));

							size_t sourceIndex = 0;

							if (stream.readBytes(rowBuffer) != rowBuffer.length) {
								// Could not read image row.
								return Optional!Image();
							}

							while (sourceIndex < rowBuffer.length) {
								// Swap around BGR -> RGB and write alpha channel.
								pixelBuffer[destinationIndex] = rowBuffer[sourceIndex + 2];
								pixelBuffer[destinationIndex + 1] = rowBuffer[sourceIndex + 1];
								pixelBuffer[destinationIndex + 2] = rowBuffer[sourceIndex];
								pixelBuffer[destinationIndex + 3] = 0xFF;
								sourceIndex += bytesPerPixel;
								destinationIndex += bytesPerPixel;
							}
						}

						return Optional!Image(image);
					}

					case 32: {
						enum bytesPerPixel = 4;

						immutable (ulong) rowWidth = (dimensions.x * bytesPerPixel);
						auto rowBuffer = new ubyte[rowWidth];

						stream.seekHead(fileHeader.fileOffset);

						foreach (i; 0 .. dimensions.y) {
							size_t destinationIndex =
								(pixelBuffer.length - (rowBuffer.length * (i + 1)));

							size_t sourceIndex = 0;

							if (stream.readBytes(rowBuffer) != rowBuffer.length) {
								// Could not read image row.
								return Optional!Image();
							}

							while (sourceIndex < rowBuffer.length) {
								// Swap around BGRA -> RGBA.
								pixelBuffer[destinationIndex] = rowBuffer[sourceIndex + 2];
								pixelBuffer[destinationIndex + 1] = rowBuffer[sourceIndex + 1];
								pixelBuffer[destinationIndex + 2] = rowBuffer[sourceIndex];
								pixelBuffer[destinationIndex + 3] = rowBuffer[sourceIndex + 3];
								sourceIndex += bytesPerPixel;
								destinationIndex += bytesPerPixel;
							}
						}

						return Optional!Image(image);
					}

					// Unsupporterd bit per pixel format.
					default: break pixelEncodingCheck;
				}
			}
			// Invalid info header, image too big, or bitmap format is compressed.
		}
		// Invalid file or info header.

		return Optional!Image();
	}

	/**
	 * Returns a non-owning view of the internal pixel data.
	 *
	 * Any subsequent mutable function called on the [Image] instance should be considered to
	 * render any existing views into its contents invalid. Because of this, it is highly advised
	 * that a handle into its memory contents not be saved anywhere but temporary storage.
	 */
	public inout (Color)[] opIndex() inout {
		return this.pixelBuffer[0 .. (this.pixelDimensions.x * this.pixelDimensions.y)];
	}
}
