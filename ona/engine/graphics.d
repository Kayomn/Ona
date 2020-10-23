module ona.engine.graphics;

public import
	ona.engine.slang;

private import
	ona.collections,
	ona.core;

/**
 * Caching structure for storing the event information about the current frame.
 */
public struct Events {
	/**
	 * The time between the current and previous frame represented as a value between `0` and `1`.
	 *
	 * This is typically used as a way to support CPU speed-independent interpolations that take
	 * place over multiple frames.
	 */
	float deltaTime;
}

public struct Property {
	PropertyType type;

	uint components;

	const (char)[] name;

	@nogc
	this(in PropertyType type, in char[] name) pure {
		this.type = type;
		this.name = name;
		this.components = 1;
	}

	@nogc
	this(in PropertyType type, in char[] name, in uint components) pure {
		this.type = type;
		this.name = name;
		this.components = components;
	}
}

/**
 * Thread-local message queue for organizing render calls into an optimized format for dispatch at
 * a later time.
 */
public interface GraphicsCommands {
	/**
	 * Dispatches the commands queued in the `GraphicsCommands` to `graphicsServer`, clearing the
	 * `GraphicsCommands` command queue in the process.
	 */
	void dispatch(GraphicsServer graphicsServer);
}

/**
 * Opaque request server to the graphics and windowing systems of a platform.
 */
public interface GraphicsServer {
	/**
	 * Clears the backbuffer of the `GraphicsServer` to black.
	 */
	@nogc
	void clear();

	/**
	 * Clears the backbuffer of the `GraphicsServer` to `color`.
	 */
	@nogc
	void coloredClear(Color color);

	/**
	 * Reads the relevant events received from the platform and writes them to `events`,
	 * returning `true` if the process is expected to continue as normal or `false` if a signal
	 * has been received to exit.
	 */
	@nogc
	bool readEvents(ref Events events);

	/**
	 * Updates the visible state of the `GraphicsServer`, swapping the back and front buffers.
	 */
	@nogc
	void update();

	/**
	 *
	 *
	 * If such a renderer does not yet exist, a new one will be created and returned.
	 *
	 * If the resources for creating the renderer cannot be acquired or the inputs are
	 * malformed, a `0` `ResourceID` is returned instead.
	 */
	@nogc
	ResourceID createRenderer(
		in Property[] vertexProperties,
		in Property[] rendererProperties,
		in Property[] materialProperties,
		in char[] shaderSource
	);

	/**
	 * Attempts to request a material that targets the renderer at `rendererID` using `mainTexture`
	 * as the main texture and `materialData` as the properties of the material.
	 *
	 * If the resources for creating the material cannot be acquired or the inputs are
	 * malformed, a `0` `ResourceID` is returned instead.
	 */
	@nogc
	ResourceID createMaterial(
		ResourceID rendererID,
		in Image mainTexture,
		scope const (void)[] materialData
	);

	/**
	 * Attempts to request a renderable poly collection that targets the renderer at
	 * `rendererID` using `vertexData` as the vertices.
	 *
	 * If the resources for creating the poly collection cannot be acquired or the inputs are
	 * malformed, a `0` `ResourceID` is returned instead.
	 */
	@nogc
	ResourceID createPoly(ResourceID rendererID, scope const (void)[] vertexData);

	/**
	 * Requests the the poly `polyId` by rendered to the `GraphicsServer` backbuffer using the
	 * material at `materialId` as its material and the renderer at `rendererId` as its
	 * renderer for `count` instances.
	 *
	 * Note that in order for `GraphicsServer.renderPolyInstanced` to work, the renderer at
	 * `rendererId` **must** support instanced rendering.
	 */
	@nogc
	void renderPolyInstanced(
		ResourceID rendererID,
		ResourceID polyID,
		ResourceID materialID,
		size_t count
	);

	/**
	 * Updates the userdata of the renderer at `rendererID` with `rendererData`.
	 */
	@nogc
	void updateRendererUserdata(ResourceID rendererID, scope const (void)[] rendererData);
}

/**
 * Runtime identifiers for renderer layout types.
 */
public enum PropertyType {
	integral,
	floating,
	matrix,
	vector2,
	vector3,
	vector4,
}

public alias ResourceID = Key!(uint);
