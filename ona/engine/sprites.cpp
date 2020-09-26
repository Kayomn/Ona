#include "ona/engine.hpp"

using Ona::Core::Assert;
using Ona::Core::BytesOf;
using Ona::Core::Chars;
using Ona::Core::CharsFrom;
using Ona::Core::Optional;
using Ona::Core::Result;
using Ona::Core::Slice;
using Ona::Core::Vector2;
using Ona::Core::Vector4;

using Ona::Engine::Attribute;
using Ona::Engine::TypeDescriptor;
using Ona::Engine::Layout;
using Ona::Engine::ResourceId;

struct Vertex2D {
	Vector2 position;

	Vector2 uv;
};

internal ResourceId spriteRendererId;

internal ResourceId spriteRectId;

internal void LazyInitSpriteRenderer(Ona::Engine::GraphicsServer * graphics) {
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
		"layout(std140, row_major) uniform Camera {\n"
		"	mat4 projectionTransform;\n"
		"};\n"
		"\n"
		"layout(std140, row_major) uniform Material {\n"
		"	vec4 tintColor;\n"
		"};\n"
		"\n"
		"layout(std140, row_major) uniform Instance {\n"
		"	mat4x4 transforms[INSTANCE_COUNT];\n"
		"	vec4 viewports[INSTANCE_COUNT];\n"
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
		Attribute{TypeDescriptor::Float, (64 * 128), CharsFrom("transforms")},
		Attribute{TypeDescriptor::Float, (16 * 128), CharsFrom("viewports")},
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

		spriteRendererId = graphics->CreateRenderer(
			vertexSource,
			fragmentSource,
			Layout{2, vertexAttributes},
			Layout{2, userdataAttributes},
			Layout{1, materialAttributes}
		).Expect(CharsFrom("Sprite renderer failed to lazily initialize"));

		spriteRectId = graphics->CreatePoly(
			spriteRendererId,
			Ona::Core::SliceOf(quadPoly, 6).AsBytes()
		).Expect(CharsFrom("Sprite rect polygon failed to lazily initialize"));
	}
}

struct Material {
	Vector4 tintColor;
};

namespace Ona::Engine {
	Optional<Sprite> CreateSprite(GraphicsServer * graphics, Ona::Core::Image const & image) {
		using Opt = Optional<Sprite>;

		LazyInitSpriteRenderer(graphics);

		let createdMaterial = graphics->CreateMaterial(spriteRendererId, image);

		if (createdMaterial.IsOk()) {
			ResourceId const materialId = createdMaterial.Value();
			Material material = {Vector4{1.f, 1.f, 1.f, 1.f}};

			graphics->UpdateMaterialUserdata(materialId, BytesOf(material));

			return Opt{Sprite{
				spriteRectId,
				materialId
			}};
		}

		return Opt{};
	}

	void Sprite::Free() {
		// TODO:
	}

	uint64_t Sprite::Hash() const {
		return ((static_cast<uint64_t>(this->polyId) << 32) |
				static_cast<uint64_t>(this->materialId));
	}

	SpriteRenderCommands::SpriteRenderCommands(GraphicsServer * graphics) : batches{} {
		LazyInitSpriteRenderer(graphics);
	}

	void SpriteRenderCommands::Dispatch(GraphicsServer * graphics) {
		this->batches.ForEach([this, &graphics](Sprite & sprite, BatchSet & batchSet) {
			Batch * batch = (&batchSet.head);

			graphics->UpdateRendererMaterial(spriteRendererId, sprite.materialId);

			while (batch && batch->count) {
				graphics->UpdateRendererUserdata(spriteRendererId, BytesOf(batch->chunk));

				graphics->RenderPolyInstanced(
					spriteRendererId,
					sprite.polyId,
					sprite.materialId,
					batch->count
				);

				batch = batch->next;
			}
		});
	}

	void SpriteRenderCommands::Draw(Sprite sprite, Vector2 position) {
		Optional<BatchSet *> batchSet = this->batches.LookupOrInsert(sprite, []() {
			BatchSet batchSet = {};
			batchSet.current = (&batchSet.head);

			return batchSet;
		});

		if (batchSet.HasValue()) {
			Batch * currentBatch = batchSet.Value()->current;

			if (currentBatch->count == Chunk::max) {
				currentBatch->next = Ona::Core::New<Batch>();
				currentBatch = currentBatch->next;
			}

			// TODO: Assign these fuckers.
			currentBatch->count += 1;
			currentBatch->chunk.transforms[0] = Ona::Core::Matrix{};
			currentBatch->chunk.viewports[0] = Ona::Core::Vector4{};
		}

		// TODO: Record failed render.
	}
}
