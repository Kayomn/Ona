#include "ona/engine.hpp"

using Ona::Core::Assert;
using Ona::Core::Chars;
using Ona::Core::CharsFrom;
using Ona::Core::Optional;
using Ona::Core::Result;
using Ona::Core::Slice;

using Ona::Engine::Attribute;
using Ona::Engine::TypeDescriptor;
using Ona::Engine::Layout;
using Ona::Engine::ResourceId;

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
		"	gl_Position = (projectionTransform * transforms[gl_InstanceID] * vec4(quadVertex, 0.0, 1.0));\n"
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
		Result<ResourceId, Ona::Engine::RendererError> result = graphics->CreateRenderer(
			vertexSource,
			fragmentSource,
			Layout{2, vertexAttributes},
			Layout{2, userdataAttributes},
			Layout{1, materialAttributes}
		);

		if (result.IsOk()) {
			spriteRendererId = result.Value();
		} else {
			spriteRendererId = 0;
			// TODO: Error.
		}
	}
}

namespace Ona::Engine {
	Optional<Sprite> CreateSprite(GraphicsServer * graphics, Ona::Core::Image const & image) {
		using Opt = Optional<Sprite>;

		LazyInitSpriteRenderer(graphics);

		let createdMaterial = graphics->CreateMaterial(spriteRendererId, image);

		if (createdMaterial.IsOk()) {
			return Opt{Sprite{
				spriteRectId,
				createdMaterial.Value()
			}};
		}

		return Opt{};
	}

	SpriteRenderCommands::SpriteRenderCommands(GraphicsServer * graphics) : renderChunks{} {
		LazyInitSpriteRenderer(graphics);
	}

	void SpriteRenderCommands::Dispatch(GraphicsServer * graphics) {
		this->renderChunks.ForEach([this, &graphics](Sprite & sprite, RenderChunk & startingChunk) {
			RenderChunk * chunk = (&startingChunk);

			graphics->UpdateRendererMaterial(spriteRendererId, sprite.materialId);

			while (chunk && chunk->count) {
				graphics->UpdateRendererUserdata(spriteRendererId, Ona::Core::BytesOf(chunk->batch));

				graphics->RenderPolyInstanced(
					spriteRendererId,
					sprite.polyId,
					sprite.materialId,
					chunk->count
				);

				chunk = chunk->next;
			}
		});
	}
}
