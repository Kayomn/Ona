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

	enum class ShaderError {
		None
	};

	enum class RendererError {
		None
	};

	enum class PolyError {
		None
	};

	enum class MaterialError {
		None,
		Server,
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

		virtual ResourceId CreateShader(Chars const & sourceCode, ShaderError * error) = 0;

		virtual ResourceId CreateRenderer(
			ResourceId shaderId,
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
			ResourceId rendererId,
			Slice<uint8_t const> const & materialData,
			Image const & texture,
			ResourceId shaderId,
			MaterialError * error
		) = 0;
	};

	GraphicsServer * LoadOpenGl(String const & title, int32_t width, int32_t height);

	void UnloadGraphics(GraphicsServer *& graphicsServer);

	Vector4 NormalizeColor(Color const & color);
}

#endif
