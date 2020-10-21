module ona.core.image;

private import
	ona.core.math,
	ona.core.memory,
	ona.core.os,
	ona.core.types;

/**
 * Red, green, blue, alpha color value.
 */
public struct Color {
	enum : Color {
		/**
		 * Black `Color`.
		 */
		black = Color(0xFF, 0xFF, 0xFF, 0xFF),

		/**
		 * White `Color`.
		 */
		white = Color(0xFF, 0xFF, 0xFF, 0xFF),
	}

	/**
	 * Red, green, blue, alpha color channels represented as a range between `ubyte.min` and
	 * `ubyte.max`.
	 */
	ubyte r, g, b, a;

	/**
	 * Converts the `Color` into a `Vector4` of normalized red, green, blue, alpha values.
	 */
	@nogc
	Vector4 normalized() const pure {
		return Vector4(
			(this.r / (cast(float)0xFF)),
			(this.g / (cast(float)0xFF)),
			(this.b / (cast(float)0xFF)),
			(this.a / (cast(float)0xFF))
		);
	}
}

/**
 * Resource wrapper for a collection of pixel data.
 */
public struct Image {
	/**
	 * Optional `Allocator` to override the global allocation strategy.
	 */
	private Allocator allocator;

	/**
	 * RGBA pixel data.
	 */
	private Color* pixels;

	/**
	 * Image width and height in RGBA pixels.
	 */
	private Point2 dimensions;

	/**
	 * Retrieves the dimensions of the `Image`.
	 */
	@nogc
	public Point2 dimensionsOf() pure const {
		return this.dimensions;
	}

	/**
	 * Frees the `Image`, provided there is something to free.
	 */
	@nogc
	public void free() {
		if (this.pixels && this.allocator) {
			this.allocator.deallocate(this.pixels);
		}
	}

	/**
	 * Attempts to allocate a solid `Image` of from `data` with the pixel dimensions of
	 * `dimensions`.
	 *
	 * A `ImageError.unsupportedFormat` error is produced if any value in `dimensions` is less than
	 * `1` or `data.length != (dimensions.x * dimensions.y)`.
	 *
	 * A `ImageError.outOfMemory` error is produced if the `Image` fails to allocate.
	 */
	@nogc
	public static Result!(Image, ImageError) from(
		Allocator allocator,
		in Point2 dimensions,
		in Color[] data
	) {
		alias Res = Result!(Image, ImageError);

		if ((dimensions.x > 0) && (dimensions.y > 0)) {
			immutable (size_t) pixelArea = (dimensions.x * dimensions.y);

			if (pixelArea == data.length) {
				ubyte[] pixelData = allocator.allocate(pixelArea * Color.sizeof);

				if (pixelData.length) {
					copyMemory(pixelData, cast(ubyte[])data);

					return Res.ok(Image(allocator, (cast(Color*)pixelData.ptr), dimensions));
				}

				return Res.fail(ImageError.outOfMemory);
			}
		}

		return Res.fail(ImageError.unsupportedFormat);
	}

	/**
	 * Attempts to allocate a solid-color `Image` of `color` with the pixel dimensions of
	 * `dimensions`.
	 *
	 * A `ImageError.unsupportedFormat` error is produced if any value in `dimensions` is less than
	 * `1`.
	 *
	 * A `ImageError.outOfMemory` error is produced if the `Image` fails to allocate.
	 */
	@nogc
	public static Result!(Image, ImageError) solid(
		Allocator allocator,
		in Point2 dimensions,
		in Color color
	) {
		alias Res = Result!(Image, ImageError);

		if ((dimensions.x > 0) && (dimensions.y > 0)) {
			immutable (size_t) pixelArea = (dimensions.x * dimensions.y);
			ubyte[] pixelData = allocator.allocate(pixelArea * Color.sizeof);

			if (pixelData.length) {
				foreach (i; 0 .. pixelArea) {
					copyMemory(pixelData[(i * Color.sizeof) .. pixelData.length], asBytes(color));
				}

				return Res.ok(Image(allocator, (cast(Color*)pixelData.ptr), dimensions));
			}

			return Res.fail(ImageError.outOfMemory);
		}

		return Res.fail(ImageError.unsupportedFormat);
	}
}

/**
 * Error codes used with `Image` creation functions.
 */
public enum ImageError {
	unsupportedFormat,
	outOfMemory
}

/**
 * Creates a greyscale `Color` of `value` represented in a range of `0` to `255`.
 */
@nogc
public Color greyscale(in ubyte value) pure {
	return Color(value, value, value, 0xFF);
}

/**
 * Creates an opaque `Color` from the color channel values `red`, `green`, and `blue`.
 */
@nogc
public Color rgb(in ubyte red, in ubyte green, in ubyte blue) pure {
	return Color(red, green, blue, 0xFF);
}

