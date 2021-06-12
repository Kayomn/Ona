module ona.graphics;

private import
	ona.collections.seq,
	ona.functional,
	ona.image,
	ona.math,
	ona.system,
	std.string;

/**
 * Sprite layout descriptor.
 */
public struct Sprite {
	/**
	 * Position and size on the screen, as described by [Rect.origin] and [Rect.extent]
	 * respectively.
	 *
	 * Typically, the [Rect.extent] value will want to be equal to the dimensions of the associated
	 * [Texture] for pixel perfectness.
	 */
	Rect destinationRect;
}

/**
 * Single-instance, hardware-accelerated graphics serving interface for creating hardware-bound
 * resources and submitting data to be rendered unto an abstract display.
 */
public final class Graphics {
	private immutable (Texture) defaultTexture;

	@nogc
	private this(immutable (Texture) defaultTexture) {
		this.defaultTexture = defaultTexture;
	}

	@nogc
	public ~this() {
		graphicsExit();
	}

	/**
	 * Attempts to load a single [Graphics] context named `title` with a dimension of `width` by
	 * `height` pixels, returning it or nothing depending on whether the required system resources
	 * are available to create it, wrapped in an [Optional].
	 */
	public static Optional!Graphics load(in string title, in int width, in int height) {
		static Graphics graphics = null;

		if (graphics is null) {
			const (char)* vendor = graphicsInit(toStringz(title), width, height);

			if (vendor) {
				immutable defaultPixel = Color.white;
				immutable (uint) whiteTextureHandle = graphicsLoadTexture(&defaultPixel, 1, 1);

				if (whiteTextureHandle) {
					graphics = new Graphics(new Texture(whiteTextureHandle, Point2.one));

					print("Loaded graphics instance: " ~ fromStringz(vendor));

					return Optional!Graphics(graphics);
				}
			}
		}

		return Optional!Graphics();
	}

	/**
	 * Attempts to allocate and create a [Texture] from `image`, returning it if the operation was
	 * successful, otherwise falling back to returning a default [Texture].
	 *
	 * Reasons for failure are specific to the graphics backend, but typically they include
	 * insuffient hardware resources or invalid pixel data formatting.
	 */
	public const (Texture) loadTexture(in Image image) {
		immutable (Point2) imageDimensions = image.dimensions;

		immutable (uint) handle =
			graphicsLoadTexture(image[].ptr, imageDimensions.x, imageDimensions.y);

		if (handle) {
			return new Texture(handle, imageDimensions);
		}

		return this.defaultTexture;
	}

	/**
	 * Polls for any accumulated events pending and dispatches them synchronously.
	 */
	@nogc
	public bool poll() {
		return graphicsPoll();
	}

	/**
	 * Presents the current backbuffer unto the display.
	 */
	@nogc
	public void present() {
		graphicsPresent();
	}

	/**
	 * Clears the backbuffer to a color value equivalent to `clearColor`.
	 */
	@nogc
	public void renderClear(in Color clearColor) {
		graphicsRenderClear(clearColor.value());
	}

	/**
	 * Dispatches the data in `sprites` to the backbuffer.
	 */
	@nogc
	public void renderSprites(in Texture spriteTexture, scope const (Sprite)[] sprites...) {
		graphicsRenderSprites(spriteTexture.handle, sprites.length, sprites.ptr);
	}
}

/**
 * Hardware-accelerated, opaque, [Image]-like resource.
 */
public final class Texture {
	private uint handle;

	private Point2 texelDimensions;

	@nogc
	private this(in uint handle, in Point2 dimensions) pure {
		this.handle = handle;
		this.texelDimensions = dimensions;
	}

	@nogc
	public ~this() {
		graphicsUnloadTexture(this.handle);
	}

	/**
	 * Returns the x and y texel dimensions of the image data.
	 *
	 * Texels are typically equivalent to [Color]-formatted pixels.
	 */
	@nogc
	public Point2 dimensions() const {
		return this.texelDimensions;
	}
}

@nogc
private extern (C) void graphicsExit();

@nogc
private extern (C) const (char)* graphicsInit(const (char)* title, int width, int height);

@nogc
private extern (C) uint graphicsLoadTexture(
	const (Color)* imagePixels,
	int imageWidth,
	int imageHeight
);

@nogc
private extern (C) bool graphicsPoll();

@nogc
private extern (C) void graphicsPresent();

@nogc
private extern (C) void graphicsRenderClear(uint color);

@nogc
private extern (C) void graphicsRenderSprites(
	uint textrueHandle,
	ulong spriteCount,
	const (Sprite)* spriteInstances
);

@nogc
private extern (C) void graphicsUnloadTexture(uint handle);
