module ona.engine.sprites;

public import
	ona.collections,
	ona.core,
	ona.engine.graphics;

/**
 * High-level abstraction of `GraphicsServer` resources used for rendering a simple 2D image to the
 * screen.
 */
public struct Sprite {
	private ResourceId polyId;

	private ResourceId materialId;

	private Vector2 dimensions;

	@nogc
	public bool opEquals(Sprite that) pure const {
		return (this.toHash() == that.toHash());
	}

	@nogc
	public ulong toHash() pure const {
		return (((cast(ulong)this.polyId) << 32) | (cast(ulong)this.materialId));
	}
}

private struct Material {
	Vector4 tintColor;
}

private struct Vertex2D {
	Vector2 position;

	Vector2 uv;
}

/**
 * Queued list of render commands to be dispatched at a later time.
 */
public class SpriteCommands : GraphicsCommands {
	private struct Batch {
		Batch* next;

		size_t count;

		Chunk chunk;
	}

	private struct Chunk {
		enum max = 128;

		Matrix projectionTransform;

		Matrix[max] transforms;

		Vector4[max] viewports;
	}

	private struct BatchSet {
		Batch* current;

		Batch head;
	}

	private Table!(Sprite, BatchSet) batches;

	private ResourceId rendererId;

	private ResourceId rectPolyId;

	/**
	 * Attempts to create a `Sprite` instance wrapped in an `Optional` from the pixel data `image`
	 * using `graphicsServer`.
	 *
	 * Should the `Sprite` fail to be created, `null` is returned instead.
	 */
	public Result!Sprite createSprite(GraphicsServer* graphicsServer, Image image) {
		alias Res = Result!Sprite;
		immutable materialId = graphicsServer.createMaterial(this.rendererId, image);

		if (materialId) {
			immutable imageSize = image.dimensionsOf();
			Material material = {tintColor: Vector4.of(1f)};

			graphicsServer.updateMaterialUserdata(materialId, DataBuffer(asBytes(material)));

			return Res.ok(Sprite(
				this.rectPolyId,
				materialId,
				Vector2(imageSize.x, imageSize.y)
			));
		}

		return Res.fail();
	}

	override void dispatch(GraphicsServer* graphicsServer) {
		foreach (ref sprite, ref batchSet; this.batches.valuesOf()) {
			Batch* batch = (&batchSet.head);

			while (batch && batch.count) {
				graphicsServer.updateRendererUserdata(
					this.rendererId,
					DataBuffer(asBytes(batch.chunk))
				);

				graphicsServer.renderPolyInstanced(
					this.rendererId,
					sprite.polyId,
					sprite.materialId,
					batch.count
				);

				batch.count = 0;
				batch = batch.next;
			}
		}

		this.batches.clear();
	}

	/**
	 * Draws `sprite` at coordinates `position`.
	 */
	public void draw(Sprite sprite, Vector2 position) {
		BatchSet* batchSet = this.batches.lookupOrInsert(sprite, () => BatchSet.init);

		if (batchSet) {
			if (!batchSet.current) batchSet.current = (&batchSet.head);

			Batch* currentBatch = batchSet.current;

			if (currentBatch.count == Chunk.max) {
				Batch* batch = (cast(Batch*)allocate(Batch.sizeof).ptr);

				if (batch) {
					(*batch) = Batch();
					currentBatch.next = batch;
					currentBatch = currentBatch.next;
				}
			}

			// TODO: Remove hardcoded viewport sizes.
			currentBatch.chunk.projectionTransform = orthographicMatrix(0, 640, 480, 0, -1, 1);

			currentBatch.chunk.transforms[currentBatch.count] = Matrix(
				sprite.dimensions.x, 0f, 0f, position.x,
				0f, sprite.dimensions.y, 0f, position.y,
				0f, 0f, 1f, 0f,
				0f, 0f, 0f, 1f
			);

			currentBatch.chunk.viewports[currentBatch.count] = Vector4(0f, 0f, 1f, 1f);
			currentBatch.count += 1;
		}

		// TODO: Record failed render.
	}

	/**
	 * Allocates and initializes a new `SpriteCommands` instances, provided it can acquire the#
	 * resources required to do so from `graphicsServer`.
	 */
	public static SpriteCommands make(GraphicsServer* graphicsServer) {
		SpriteCommands commands = new SpriteCommands();

		if (commands) {
			static immutable vertexSource = `
#version 430 core
#define INSTANCE_COUNT 128

in vec2 quadVertex;
in vec2 quadUv;

out vec2 texCoords;
out vec4 texTint;

layout(std140, row_major) uniform Renderer {
	mat4x4 projectionTransform;
	mat4x4 transforms[INSTANCE_COUNT];
	vec4 viewports[INSTANCE_COUNT];
};

layout(std140, row_major) uniform Material {
	vec4 tintColor;
};

uniform sampler2D spriteTexture;

void main() {
	const vec4 viewport = viewports[gl_InstanceID];

	texCoords = ((quadUv * viewport.zw) + viewport.xy);
	texTint = tintColor;

	gl_Position = (
		projectionTransform * transforms[gl_InstanceID] * vec4(quadVertex, 0.0, 1.0)
	);
}`;

			static immutable fragmentSource = `
#version 430 core

in vec2 texCoords;
in vec4 texTint;
out vec4 outColor;

uniform sampler2D spriteTexture;

void main() {
	const vec4 spriteTextureColor = (texture(spriteTexture, texCoords) * texTint);

	if (spriteTextureColor.a == 0.0) discard;

	outColor = spriteTextureColor;
}`;

			static immutable vertexAttributes = [
				Attribute(TypeDescriptor.float_, 2),
				Attribute(TypeDescriptor.float_, 2)
			];

			static immutable userdataAttributes = [
				Attribute(TypeDescriptor.float_, 16),
				Attribute(TypeDescriptor.float_, (16 * 128)),
				Attribute(TypeDescriptor.float_, (4 * 128)),
			];

			static immutable materialAttributes = [
				Attribute(TypeDescriptor.float_, 4)
			];

			commands.rendererId = graphicsServer.requestRenderer(
				Chars.parseUtf8(vertexSource),
				Chars.parseUtf8(fragmentSource),
				Layout(vertexAttributes),
				Layout(userdataAttributes),
				Layout(materialAttributes)
			);

			static immutable quadPoly = [
				Vertex2D(Vector2(1f, 1f), Vector2(1f, 1f)),
				Vertex2D(Vector2(1f, 0f), Vector2(1f, 0f)),
				Vertex2D(Vector2(0f, 1f), Vector2(0f, 1f)),
				Vertex2D(Vector2(1f, 0f), Vector2(1f, 0f)),
				Vertex2D(Vector2(0f, 0f), Vector2(0f, 0f)),
				Vertex2D(Vector2(0f, 1f), Vector2(0f, 1f))
			];

			commands.rectPolyId = graphicsServer.requestPoly(
				commands.rendererId,
				DataBuffer(cast(ubyte[])quadPoly)
			);

			if (commands.rendererId && commands.rectPolyId) return commands;
		}

		return null;
	}
}
