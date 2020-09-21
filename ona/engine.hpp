#ifndef ENGINE_H
#define ENGINE_H

#include "ona/core.hpp"
#include "ona/collections.hpp"

namespace Ona::Engine {
	using Ona::Core::Color;
	using Ona::Core::String;
	using Ona::Core::Vector4;
	using Ona::Core::Slice;
	using Ona::Core::Image;
	using Ona::Core::Chars;

	using ResourceId = uint32_t;

	struct Events {
		float deltaTime;
	};

	enum class TypeDescriptor : uint16_t {
		Byte,
		UnsignedByte,
		Short,
		UnsignedShort,
		Int,
		UnsignedInt,
		Float,
		Double
	};

	struct Attribute {
		uint8_t location;

		TypeDescriptor type;

		uint16_t components;

		Chars name;

		uint32_t offset;
	};

	enum ShaderType {
		ShaderTypeEmpty = 0,
		ShaderTypeVertex = 0x1,
		ShaderTypeFragment = 0x2
	};

	struct ShaderSource {
		ShaderType type;

		Chars text;
	};

	struct MaterialLayout {
		Slice<Attribute> properties;

		bool Validate(Slice<uint8_t const> const & data) const;
	};

	struct VertexLayout {
		uint32_t vertexSize;

		Slice<Attribute> attributes;

		bool Validate(Slice<uint8_t const> const & data) const;
	};

	struct GraphicsCommands {

	};

	enum class RendererError {
		None
	};

	enum class PolyError {
		None,
		Server,
		BadLayout,
		BadRenderer,
		BadVertices
	};

	enum class MaterialError {
		None,
		Server,
		BadLayout,
		BadRenderer,
		BadImage
	};

	class GraphicsServer {
		public:
		virtual ~GraphicsServer() { }

		virtual void Clear() = 0;

		virtual void ColoredClear(Color color) = 0;

		virtual void SubmitCommands(GraphicsCommands const & commands) = 0;

		virtual bool ReadEvents(Events & events) = 0;

		virtual void Update() = 0;

		virtual ResourceId CreateRenderer(
			Slice<ShaderSource> const & shaderSources,
			MaterialLayout const & materialLayout,
			VertexLayout const & vertexLayout,
			RendererError * error
		) = 0;

		virtual ResourceId CreatePoly(
			ResourceId rendererId,
			Slice<uint8_t const> const & vertexData,
			PolyError * error
		) = 0;

		virtual ResourceId CreateMaterial(
			Slice<ShaderSource> const & shaderSources,
			ResourceId rendererId,
			Slice<uint8_t const> const & materialData,
			Image const & texture,
			MaterialError * error
		) = 0;
	};

	GraphicsServer * LoadOpenGl(String const & title, int32_t width, int32_t height);

	void UnloadGraphics(GraphicsServer * & graphicsServer);

	Vector4 NormalizeColor(Color const & color);
}

#endif
