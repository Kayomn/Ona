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

	/**
	 * Returns a `Sprite` centered around `dimensions` size at `position`.
	 */
	static Sprite centered(in Vector2 dimensions, in Vector2 position) {
		return Sprite(Rect(position - (dimensions / 2), dimensions));
	}
}

/**
 * Single-instance, hardware-accelerated graphics serving interface for creating hardware-bound
 * resources and submitting data to be rendered unto an abstract display.
 */
public final class Graphics {
	private immutable (Texture) defaultTexture;

	private this(immutable (Texture) defaultTexture) {
		this.defaultTexture = defaultTexture;
	}

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
	public bool poll() {
		return graphicsPoll();
	}

	/**
	 * Presents the current backbuffer unto the display.
	 */
	public void present() {
		graphicsPresent();
	}

	/**
	 * Clears the backbuffer to a color value equivalent to `clearColor`.
	 */
	public void renderClear(in Color clearColor) {
		graphicsRenderClear(clearColor.value());
	}

	/**
	 * Dispatches the data in `sprites` to the backbuffer.
	 */
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

	private this(in uint handle, in Point2 dimensions) pure {
		this.handle = handle;
		this.texelDimensions = dimensions;
	}

	public ~this() {
		graphicsUnloadTexture(this.handle);
	}

	/**
	 * Returns the x and y texel dimensions of the image data.
	 *
	 * Texels are typically equivalent to [Color]-formatted pixels.
	 */
	public Point2 dimensions() const {
		return this.texelDimensions;
	}
}

private extern (C) void graphicsExit();

private extern (C) const (char)* graphicsInit(const (char)* title, int width, int height);

private extern (C) uint graphicsLoadTexture(
	const (Color)* imagePixels,
	int imageWidth,
	int imageHeight
);

private extern (C) bool graphicsPoll();

private extern (C) void graphicsPresent();

private extern (C) void graphicsRenderClear(uint color);

private extern (C) void graphicsRenderSprites(
	uint textrueHandle,
	ulong spriteCount,
	const (Sprite)* spriteInstances
);

private extern (C) void graphicsUnloadTexture(uint handle);
