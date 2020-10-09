module ona.engine.graphics;

private import
	ona.collections,
	ona.core;

/**
 * Represents a field or property of data used by `GraphicsServer`-based resources, including
 * renderer userdata, material userdata, and vertex layout.
 */
public struct Attribute {
	/**
	 * Runtime type descriptor used to represent the type.
	 */
	TypeDescriptor type;

	/**
	 * Number of components that compose the `Attribute`. This is used for vector, matrix, and array
	 * types.
	 *
	 * Examples of how various Ona types would be expressed in component format:
	 *
	 *   * `vector3` -> `3`
	 *   * `point2[10]` -> `2 * 10` or `20`
	 *   * `matrix` -> `16`
	 *   * `matrix[2]` -> `16 * 2` or `32`
	 */
	uint components;

	/**
	 * Name of the `Attribute` used within the shader program.
	 */
	Chars name;

	/**
	 * Calculates the number of bytes that compose the `Attribute` based on the `TypeDescriptor` and
	 * number of components.
	 */
	@nogc
	size_t byteSize() const pure {
		size_t size = void;

		typeSize: final switch (this.type) {
			case TypeDescriptor.byte_, TypeDescriptor.ubyte_: {
				size = 1;
			} break typeSize;

			case TypeDescriptor.short_, TypeDescriptor.ushort_: {
				size = 2;
			} break typeSize;

			case TypeDescriptor.int_, TypeDescriptor.uint_: {
				size = 4;
			} break typeSize;

			case TypeDescriptor.float_: {
				size = 4;
			} break typeSize;

			case TypeDescriptor.double_: {
				size = 8;
			} break typeSize;
		}

		return (size * this.components);
	}
}

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

/**
 * Language-agnostic sequence of `Attribute`s.
 */
public const struct Layout {
	/**
	 * Number of `Attribute`s in the `Layout`.
	 */
	size_t length;

	/**
	 * Pointer to the first `Attribute`.
	 */
	Attribute* pointer;

	/**
	 * Constructs a `Layout` from `attributes`.
	 */
	@nogc
	this(const (Attribute)[] attributes) pure {
		this.length = attributes.length;
		this.pointer = attributes.ptr;
	}
}

/**
 * Language-agnostic type-erasing wrapper around a sequence of bytes.
 */
public const struct DataBuffer {
	/**
	 * Number of bytes in the `DataBuffer`.
	 */
	size_t length;

	/**
	 * Pointer to the first byte.
	 */
	ubyte* pointer;

	/**
	 * Constructs a `DataBuffer` from `data`.
	 */
	@nogc
	this(const (ubyte)[] data) pure {
		this.length = data.length;
		this.pointer = data.ptr;
	}
}

/**
 * Opaque interface to a platform-specific hardware-accelerated graphics API.
 */
public struct GraphicsServer {
	private void* instance;

	extern (C) {
		@nogc
		private void function(void* instance) clearer;

		@nogc
		private void function(void* instance, Color color) coloredClearer;

		@nogc
		private bool function(void* instance, Events* events) eventsReader;

		@nogc
		private void function(void* instance) updater;

		@nogc
		private ResourceId function(
			void* instance,
			Chars vertexSource,
			Chars fragmentSource,
			Layout vertexLayout,
			Layout userdataLayout,
			Layout materialLayout
		) rendererRequester;

		@nogc
		private ResourceId function(
			void* instance,
			ResourceId rendererId,
			DataBuffer vertexData
		) polyRequester;

		@nogc
		private ResourceId function(
			void* instance,
			ResourceId rendererId,
			Image texture
		) materialCreator;

		@nogc
		private void function(
			void* instance,
			ResourceId rendererId,
			ResourceId polyId,
			ResourceId materialId,
			size_t count
		) instancedPolyRenderer;

		@nogc
		private void function(
			void* instance,
			ResourceId materialId,
			DataBuffer userdata
		) materialUserdataUpdater;

		@nogc
		private void function(
			void* instance,
			ResourceId rendererId,
			DataBuffer userdata
		) rendererUserdataUpdater;
	}

	/**
	 * Clears the backbuffer.
	 */
	@nogc
	public void clear() {
		if (this.clearer) this.clearer(this.instance);
	}

	/**
	 * Clears the backbuffer to the color value `color`.
	 */
	@nogc
	void coloredClear(Color color) {
		if (this.coloredClearer) this.coloredClearer(this.instance, color);
	}

	/**
	 * Reads events from the `GraphicsServer` backend and writes them to the standardized format
	 * represented in `Events`.
	 */
	@nogc
	bool readEvents(Events* events) {
		return (this.eventsReader ? this.eventsReader(this.instance, events) : false);
	}

	/**
	 * Swaps the buffers around, presenting whatever has been rendered to the backbuffer.
	 */
	@nogc
	void update() {
		if (this.updater) this.updater(this.instance);
	}

	/**
	 * Attempts to request a renderer resource from the `GraphicsServer`.
	 *
	 * If no such resource already exists, a new will be created.
	 */
	@nogc
	ResourceId requestRenderer(
		Chars vertexSource,
		Chars fragmentSource,
		Layout vertexLayout,
		Layout userdataLayout,
		Layout materialLayout
	) {
		return (this.rendererRequester ? this.rendererRequester(
			this.instance,
			vertexSource,
			fragmentSource,
			vertexLayout,
			userdataLayout,
			materialLayout
		) : 0);
	}

	/**
	 * Attempts to request a poly resource from the `GraphicsServer` that belongs to the renderer
	 * referenced by `rendererId` and has identical vertex data to `vertexData`.
	 *
	 * If no such resource already exists, a new will be created.
	 */
	@nogc
	ResourceId requestPoly(ResourceId rendererId, DataBuffer vertexData) {
		return (this.polyRequester ? this.polyRequester(this.instance, rendererId, vertexData) : 0);
	}

	/**
	 * Attempts to create a new material resource on the `GraphicsServer`.
	 */
	@nogc
	ResourceId createMaterial(ResourceId rendererId, Image texture) {
		return (
			this.materialCreator ?
			this.materialCreator(this.instance, rendererId, texture) :
			0
		);
	}

	/**
	 * Requests that the `GraphicsServer` render the poly referenced by `polyId` using the
	 * material referenced by `materialId` using the renderer referenced by `rendererId` `count`
	 * number of times.
	 *
	 * Note that in order for `GraphicsServer.renderPolyInstanced` to work, the renderer
	 * referenced by `rendererId` **must** support instanced rendering.
	 */
	@nogc
	void renderPolyInstanced(
		ResourceId rendererId,
		ResourceId polyId,
		ResourceId materialId,
		size_t count
	) {
		if (this.instancedPolyRenderer) {
			this.instancedPolyRenderer(this.instance, rendererId, polyId, materialId, count);
		}
	}

	/**
	 * Updates the userdata stored in the material referenced by `materialId` with `userdata`,
	 * provided that it is deemed to be valid data.
	 */
	@nogc
	void updateMaterialUserdata(ResourceId materialId, DataBuffer userdata) {
		if (this.materialUserdataUpdater) {
			this.materialUserdataUpdater(this.instance, materialId, userdata);
		}
	}

	/**
	 * Updates the userdata stored in the renderer referenced by `rendererId` with `userdata`,
	 * provided that it is deemed to be valid data.
	 */
	@nogc
	void updateRendererUserdata(ResourceId rendererId, DataBuffer userdata) {
		if (this.rendererUserdataUpdater) {
			this.rendererUserdataUpdater(this.instance, rendererId, userdata);
		}
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
	@nogc
	void dispatch(GraphicsServer* graphicsServer);
}

/**
 * Error codes used when a `GraphicsServer` fails to produce a valid renderer resource.
 */
public enum RendererError {
	server,
	badShader
}

/**
 * Runtime type identifiers for interfacing native code with `GraphicsServer`.
 */
public enum TypeDescriptor {
	byte_,
	ubyte_,
	short_,
	ushort_,
	int_,
	uint_,
	float_,
	double_
}

/**
 * Error codes used when a `GraphicsServer` fails to produce a valid poly resource.
 */
public enum PolyError {
	server,
	badRenderer,
	badVertices
}

/**
 * Error codes used when a `GraphicsServer` fails to produce a valid material resource.
 */
public enum MaterialError {
	server,
	badRenderer,
	badImage
}

public alias ResourceId = uint;

/**
 * Loads the OpenGL graphics backend into memory.
 */
public extern (C) GraphicsServer* loadGraphics(Chars title, int width, int height);
