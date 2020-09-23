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
			"#version 430 core"
			"#define INSTANCE_COUNT 128"
			""
			"in vec2 quadVertex;"
			"in vec2 quadUv;"
			""
			"out vec2 texCoords;"
			"out vec4 texTint;"
			""
			"layout(std140, row_major) uniform Camera {"
			"	mat4 projectionTransform;"
			"};"
			""
			"layout(std140, row_major) uniform Material {"
			"	vec4 tintColor;"
			"};"
			""
			"layout(std140, row_major) uniform Instance {"
			"	mat4x4 transforms[INSTANCE_COUNT];"
			"	vec4 viewports[INSTANCE_COUNT];"
			"};"
			""
			"uniform sampler2D spriteTexture;"
			""
			"void main() {"
			"	const vec4 viewport = viewports[gl_InstanceID];"
			""
			"	texCoords = ((quadUv * viewport.zw) + viewport.xy);"
			"	texTint = tintColor;"
			"	gl_Position = (projectionTransform * transforms[gl_InstanceID] * vec4(quadVertex, 0.0, 1.0));"
			"}"
		);

		static Chars const fragmentSource = CharsFrom(
			"#version 430 core"
			""
			"in vec2 texCoords;"
			"in vec4 texTint;"
			"out vec4 outColor;"
			""
			"uniform sampler2D spriteTexture;"
			""
			"void main() {"
			"	const vec4 spriteTextureColor = (texture(spriteTexture, texCoords) * texTint);"
			""
			"	if (spriteTextureColor.a == 0.0) discard;"
			""
			"	outColor = spriteTextureColor;"
			"}"
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
