#include "ona/engine/header.hpp"

namespace Ona::Engine {
	using Ona::Core::AsBytes;
	using Ona::Core::CharsFrom;
	using Ona::Core::Vector2;

	struct Vertex2D {
		Vector2 position;

		Vector2 uv;
	};

	internal ResourceId spriteRendererId;

	internal ResourceId spriteRectId;

	internal bool LazyInitSpriteRenderer(Ona::Engine::GraphicsServer * graphics) {
		static Chars const vertexSource = CharsFrom(
			"#version 430 core\n"
			"#define INSTANCE_COUNT 128\n"
			"\n"
			"in vec2 quadVertex;\n"
			"in vec2 quadUv;\n"
			"\n"
			"out vec2 texCoords;\n"
			"out vec4 texTint;\n"
			"\n"
			"layout(std140, row_major) uniform Renderer {\n"
			"	mat4x4 projectionTransform;\n"
			"	mat4x4 transforms[INSTANCE_COUNT];\n"
			"	vec4 viewports[INSTANCE_COUNT];\n"
			"};\n"
			"\n"
			"layout(std140, row_major) uniform Material {\n"
			"	vec4 tintColor;\n"
			"};\n"
			"\n"
			"uniform sampler2D spriteTexture;\n"
			"\n"
			"void main() {\n"
			"	const vec4 viewport = viewports[gl_InstanceID];\n"
			"\n"
			"	texCoords = ((quadUv * viewport.zw) + viewport.xy);\n"
			"	texTint = tintColor;\n"
			"\n"
			"	gl_Position = (\n"
			"		projectionTransform * transforms[gl_InstanceID] * vec4(quadVertex, 0.0, 1.0)"
			"	);\n"
			"}\n"
		);

		static Chars const fragmentSource = CharsFrom(
			"#version 430 core\n"
			"\n"
			"in vec2 texCoords;\n"
			"in vec4 texTint;\n"
			"out vec4 outColor;\n"
			"\n"
			"uniform sampler2D spriteTexture;\n"
			"\n"
			"void main() {\n"
			"	const vec4 spriteTextureColor = (texture(spriteTexture, texCoords) * texTint);\n"
			"\n"
			"	if (spriteTextureColor.a == 0.0) discard;\n"
			"\n"
			"	outColor = spriteTextureColor;\n"
			"}\n"
		);

		static Attribute const vertexAttributes[] = {
			Attribute{TypeDescriptor::Float, 2, CharsFrom("quadVertex")},
			Attribute{TypeDescriptor::Float, 2, CharsFrom("quadUv")}
		};

		static Attribute const userdataAttributes[] = {
			Attribute{TypeDescriptor::Float, 16, CharsFrom("projectionTransform")},
			Attribute{TypeDescriptor::Float, (16 * 128), CharsFrom("transforms")},
			Attribute{TypeDescriptor::Float, (4 * 128), CharsFrom("viewports")},
		};

		static Attribute const materialAttributes[] = {
			Attribute{TypeDescriptor::Float, 4, CharsFrom("tintColor")}
		};

		if (!spriteRendererId) {
			static Vertex2D quadPoly[] = {
				Vertex2D{Vector2{1.f, 1.f}, Vector2{1.f, 1.f}},
				Vertex2D{Vector2{1.f, 0.f}, Vector2{1.f, 0.f}},
				Vertex2D{Vector2{0.f, 1.f}, Vector2{0.f, 1.f}},
				Vertex2D{Vector2{1.f, 0.f}, Vector2{1.f, 0.f}},
				Vertex2D{Vector2{0.f, 0.f}, Vector2{0.f, 0.f}},
				Vertex2D{Vector2{0.f, 1.f}, Vector2{0.f, 1.f}}
			};

			let createdRendererId = graphics->CreateRenderer(
				vertexSource,
				fragmentSource,
				Layout{2, vertexAttributes},
				Layout{3, userdataAttributes},
				Layout{1, materialAttributes}
			);

			if (createdRendererId.IsOk()) {
				spriteRendererId = createdRendererId.Value();

				let createdPolyId = graphics->CreatePoly(
					spriteRendererId,
					Ona::Core::SliceOf(quadPoly, 6).AsBytes()
				);

				if (createdPolyId.IsOk()) {
					spriteRectId = createdPolyId.Value();

					return true;
				}
			}

			return false;
		}

		return true;
	}

	struct Material {
		Vector4 tintColor;
	};

	Optional<Sprite> CreateSprite(GraphicsServer * graphics, Ona::Core::Image const & image) {
		LazyInitSpriteRenderer(graphics);

		let createdMaterial = graphics->CreateMaterial(spriteRendererId, image);

		if (createdMaterial.IsOk()) {
			ResourceId const materialId = createdMaterial.Value();
			Material material = {Vector4{1.f, 1.f, 1.f, 1.f}};

			graphics->UpdateMaterialUserdata(materialId, AsBytes(material));

			return Sprite{
				.polyId = spriteRectId,
				.materialId = materialId,

				.dimensions = Vector2{
					static_cast<float>(image.dimensions.x),
					static_cast<float>(image.dimensions.y)
				}
			};
		}

		return Ona::Core::nil<Sprite>;
	}

	void Sprite::Free() {
		// TODO:
	}

	uint64_t Sprite::Hash() const {
		return ((static_cast<uint64_t>(this->polyId) << 32) |
				static_cast<uint64_t>(this->materialId));
	}

	void SpriteCommands::Dispatch(GraphicsServer * graphics) {
		this->batches.ForEach([this, &graphics](Sprite & sprite, BatchSet & batchSet) {
			let batch = (&batchSet.head);

			while (batch && batch->count) {
				graphics->UpdateRendererUserdata(spriteRendererId, AsBytes(batch->chunk));

				graphics->RenderPolyInstanced(
					spriteRendererId,
					sprite.polyId,
					sprite.materialId,
					batch->count
				);

				batch->count = 0;
				batch = batch->next;
			}
		});
	}

	void SpriteCommands::Draw(Sprite sprite, Vector2 position) {
		Optional<BatchSet *> batchSet = this->batches.LookupOrInsert(sprite, []() {
			return BatchSet{};
		});

		if (batchSet.HasValue()) {
			if (!batchSet->current.HasValue()) {
				batchSet->current = (&batchSet->head);
			}

			let currentBatch = batchSet->current;

			if (currentBatch->count == Chunk::max) {
				let allocation = Ona::Core::Allocate(sizeof(Batch));

				if (allocation.HasValue()) {
					let batch = reinterpret_cast<Batch *>(allocation.pointer);
					(*batch) = Batch{};
					currentBatch->next = batch;
					currentBatch = currentBatch->next;
				}
			}

			// TODO: Remove hardcoded viewport sizes.
			currentBatch->chunk.projectionTransform = Ona::Core::OrthographicMatrix(
				0,
				640,
				480,
				0,
				-1,
				1
			);

			currentBatch->chunk.transforms[currentBatch->count] = Matrix{
				sprite.dimensions.x, 0.f, 0.f, position.x,
				0.f, sprite.dimensions.y, 0.f, position.y,
				0.f, 0.f, 1.f, 0.f,
				0.f, 0.f, 0.f, 1.f
			};

			currentBatch->chunk.viewports[currentBatch->count] = Vector4{0.f, 0.f, 1.f, 1.f};
			currentBatch->count += 1;
		}

		// TODO: Record failed render.
	}

	bool SpriteCommands::Load(GraphicsServer * graphics) {
		return LazyInitSpriteRenderer(graphics);
	}
}
