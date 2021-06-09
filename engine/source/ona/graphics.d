module ona.graphics;

private import ona.functional, ona.image, ona.math, ona.stream, ona.system, std.string;

public final class Canvas {
	private struct Sprite {
		uint textureHandle;
	}

	private Sprite[] sprites = [];

	private Matrix[] transforms = [];

	public void drawSprite(in Texture texture, in Vector2 position) {
		auto rect = Rect(position, Vector2(texture.dimensions.x, texture.dimensions.y));

		graphicsRenderSprites(texture.handle, 1, &rect);

	}
}

public final class Graphics {
	private Texture defaultTexture;

	@trusted @nogc
	private this(in Texture defaultTexture) {
		this.defaultTexture = defaultTexture;
	}

	@system @nogc
	public ~this() {
		graphicsDispose();
	}

	/**
	 * Clears the graphics display to an equivalent color value of `clearColor`.
	 */
	@system @nogc
	public void clear(in Color clearColor) {
		graphicsClear(clearColor.value());
	}

	/**
	 * Attempts to load a single [Graphics] context named `title` with a dimension of `width` by
	 * `height` pixels, returning it or nothing depending on whether the required system resources
	 * are available to create it, wrapped in an [Optional].
	 */
	@system
	public static Optional!Graphics load(in string title, in int width, in int height) {
		static Graphics graphics = null;

		if (graphics is null) {
			const (char)* vendor = graphicsLoad(toStringz(title), width, height);

			if (vendor) {
				immutable defaultPixel = Color.white;
				immutable (uint) whiteTextureHandle = graphicsLoadTexture(&defaultPixel, 1, 1);

				if (whiteTextureHandle) {
					graphics = new Graphics(Texture(whiteTextureHandle, Point2.one));

					print("Loaded graphics instance: " ~ fromStringz(vendor));

					return Optional!Graphics(graphics);
				}
			}
		}

		return Optional!Graphics();
	}

	@system
	public Texture loadTexture(in Image image) {
		immutable (Point2) imageDimensions = image.dimensions;

		immutable (uint) handle =
			graphicsLoadTexture(image.pixels().ptr, imageDimensions.x, imageDimensions.y);

		if (handle) {
			return Texture(handle, imageDimensions);
		}

		return this.defaultTexture;
	}

	/**
	 * Polls for any accumulated events pending and dispatches them synchronously.
	 */
	@system @nogc
	public bool poll() {
		return graphicsPoll();
	}

	@system @nogc
	public void render(in Canvas canvas) {
		graphicsPresent();
	}
}

public struct Texture {
	private uint handle;

	private Point2 dimensions;

	@disable
	this();

	@safe @nogc
	private this(in uint handle, in Point2 dimensions) pure {
		this.handle = handle;
		this.dimensions = dimensions;
	}

	@system @nogc
	public void close() {
		graphicsUnloadTexture(this.handle);
	}
}

@system @nogc
private extern (C) void graphicsClear(uint color);

@system @nogc
private extern (C) void graphicsDispose();

@system @nogc
private extern (C) const (char)* graphicsLoad(const (char)* title, int width, int height);

@system @nogc
private extern (C) uint graphicsLoadTexture(
	const (Color)* imagePixels,
	int imageWidth,
	int imageHeight
);

@system @nogc
private extern (C) bool graphicsPoll();

@system @nogc
private extern (C) void graphicsPresent();

@system @nogc
private extern (C) void graphicsRenderSprites(
	uint textureHandle,
	uint instanceCount,
	const (Rect)* instanceRects
);

@system @nogc
private extern (C) void graphicsUnloadTexture(uint handle);
