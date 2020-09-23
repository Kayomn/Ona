#include "ona/engine.hpp"

using Ona::Core::Assert;
using Ona::Core::Chars;
using Ona::Core::CharsFrom;
using Ona::Core::Result;
using Ona::Core::Slice;

namespace Ona::Engine {
	constexpr char const * badGraphics = "Bad graphics server";

	Result<SpriteRenderer, RendererError> CreateSpriteRenderer(GraphicsServer * graphicsServer) {
		using Res = Result<SpriteRenderer, RendererError>;

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

		static Attribute const materialAttributes[] = {
			Attribute{TypeDescriptor::Float, 4, CharsFrom("tintColor")}
		};

		static Attribute const vertexAttributes[] = {
			Attribute{TypeDescriptor::Float, 2, CharsFrom("quadVertex")},
			Attribute{TypeDescriptor::Float, 2, CharsFrom("quadUv")}
		};

		Assert((graphicsServer != nullptr), badGraphics);

		Result<ResourceId, RendererError> result = graphicsServer->CreateRenderer(
			vertexSource,
			fragmentSource,
			Layout{1, materialAttributes},
			Layout{2, vertexAttributes}
		);

		return (
			result.IsOk() ?
			Res::Ok(SpriteRenderer{result.Value()}) :
			Res::Fail(result.Error())
		);
	}
}
