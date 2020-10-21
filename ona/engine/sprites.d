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
	private ResourceID polyID;

	private ResourceID materialID;

	private Vector2 dimensions;

	@nogc
	public bool opEquals(Sprite that) pure const {
		return (this.toHash() == that.toHash());
	}

	@nogc
	public ulong toHash() pure const {
		return (
			((cast(ulong)this.polyID.indexOf()) << 32) |
			(cast(ulong)this.materialID.indexOf())
		);
	}
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

	public this() {
		this.batches = new Table!(Sprite, BatchSet)(globalAllocator());
	}

	/**
	 * Attempts to create a `Sprite` instance wrapped in a `Result` from the pixel data `image`
	 * using `graphicsServer`.
	 *
	 * Should the `Sprite` fail to be created, an erroenous `Result` is returned instead.
	 */
	public Result!Sprite createSprite(GraphicsServer graphicsServer, Image image) {
		alias Res = Result!Sprite;

		immutable materialID = graphicsServer.createMaterial(rendererID, image, [
			Vector4.of(1f)
		]);

		if (materialID) {
			immutable imageSize = image.dimensionsOf();

			return Res.ok(Sprite(
				rectPolyID,
				materialID,
				Vector2(imageSize.x, imageSize.y)
			));
		}

		return Res.fail();
	}

	override void dispatch(GraphicsServer graphicsServer) {
		foreach (ref sprite, ref batchSet; this.batches.itemsOf()) {
			Batch* batch = (&batchSet.head);

			while (batch && batch.count) {
				graphicsServer.updateRendererUserdata(rendererID, asBytes(batch.chunk));

				graphicsServer.renderPolyInstanced(
					rendererID,
					sprite.polyID,
					sprite.materialID,
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
	public void draw(in Sprite sprite, Vector2 position) {
		BatchSet* batchSet = this.batches.require(sprite, () => BatchSet.init);

		if (batchSet) {
			if (!batchSet.current) batchSet.current = (&batchSet.head);

			Batch* currentBatch = batchSet.current;

			if (currentBatch.count == Chunk.max) {
				Batch* batch = (cast(Batch*)globalAllocator().allocate(Batch.sizeof).ptr);

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
	}

	/**
	 * Allocates and initializes a new `SpriteCommands` instances, provided it can acquire the#
	 * resources required to do so from `graphicsServer`.
	 */
	public static SpriteCommands make(GraphicsServer graphicsServer) {
		SpriteCommands commands = new SpriteCommands();

		if (commands) {
			if (rendererID && rectPolyID) {
				return commands;
			} else {
				rendererID = graphicsServer.createRenderer(
					[
						Property(PropertyType.vector2, "quadVertex"),
						Property(PropertyType.vector2, "quadUV")
					],

					[
						Property(PropertyType.matrix, "projectionTransform"),
						Property(PropertyType.matrix, "transforms", 128),
						Property(PropertyType.vector4, "viewports", 128)
					],

					[
						Property(PropertyType.vector2, "tintColor")
					],

`vector2 texCoords;

vector4 vertex() {
	const vector4 viewport = viewports[instance];
	texCoords = ((quadUV * viewport.zw) + viewport.xy);

	return (projectionTransform * transforms[instance] * vector4(quadVertex, 0.0, 1.0));
}

vector4 fragment() {
	const vector4 spriteTextureColor = (texture(mainTexture, texCoords) * tintColor);

	if (spriteTextureColor.a == 0.0) discard;

	return spriteTextureColor;
}`
				);

				rectPolyID = graphicsServer.createPoly(rendererID, [
					Vertex2D(Vector2(1f, 1f), Vector2(1f, 1f)),
					Vertex2D(Vector2(1f, 0f), Vector2(1f, 0f)),
					Vertex2D(Vector2(0f, 1f), Vector2(0f, 1f)),
					Vertex2D(Vector2(1f, 0f), Vector2(1f, 0f)),
					Vertex2D(Vector2(0f, 0f), Vector2(0f, 0f)),
					Vertex2D(Vector2(0f, 1f), Vector2(0f, 1f))
				]);

				if (rendererID && rectPolyID) return commands;
			}
		}

		return null;
	}
}

private shared (ResourceID) rendererID;

private shared (ResourceID) rectPolyID;
