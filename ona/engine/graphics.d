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

extern (C++) {
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
		bool readEvents(Events* events);

		/**
		 * Updates the visible state of the `GraphicsServer`, swapping the back and front buffers.
		 */
		@nogc
		void update();

		/**
		 * Requests a renderer from the `GraphicsServer` that uses the shader scripts `vertexSource`
		 * and `fragmentSource`, as well as the layouts `vertexLayout`, `rendererLayout`, and
		 * `materialLayout` for vertices, renderer userdata, and material userdata respectively.
		 *
		 * If such a renderer does not yet exist, a new one will be created and returned.
		 *
		 * If the resources for creating the renderer cannot be acquired or the inputs are
		 * malformed, a `0` `ResourceId` will be returned instead.
		 */
		@nogc
		ResourceId requestRenderer(
			Chars vertexSource,
			Chars fragmentSource,
			Layout vertexLayout,
			Layout rendererLayout,
			Layout materialLayout
		);

		/**
		 * Requests a material from the `GraphicsServer` that uses the renderer at `rendererId` as
		 * its target renderer, `texture` as its pixel data, and `userdata` as its userdata.
		 *
		 * If such a material does not yet exist, a new one will be created and returned.
		 *
		 * If the resources for creating the material cannot be acquired or the inputs are
		 * malformed, a `0` `ResourceId` will be returned instead.
		 */
		@nogc
		ResourceId requestMaterial(ResourceId rendererId, Image texture, DataBuffer userdata);

		/**
		 * Requests a poly collection from the `GraphicsServer` that uses the renderer at
		 * `rendererId` as its target renderer and `vertexData` as its vertex data.
		 *
		 * If such a poly collection does not yet exist, a new one will be created and returned.
		 *
		 * If the resources for creating the poly collection cannot be acquired or the inputs are
		 * malformed, a `0` `ResourceId` will be returned instead.
		 */
		@nogc
		ResourceId requestPoly(ResourceId rendererId, DataBuffer vertexData);

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
			ResourceId rendererId,
			ResourceId polyId,
			ResourceId materialId,
			size_t count
		);

		/**
		 * Updates the userdata of the renderer at `rendererId` with `userdata`.
		 */
		@nogc
		void updateRendererUserdata(ResourceId rendererId, DataBuffer userdata);
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
	void dispatch(GraphicsServer graphicsServer);
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

public alias ResourceId = uint;

/**
 * Loads the OpenGL graphics backend into memory.
 */
public extern (C) GraphicsServer loadGraphics(Chars title, int width, int height);
