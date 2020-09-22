#ifndef ENGINE_H
#define ENGINE_H

#include "ona/core.hpp"
#include "ona/collections.hpp"

namespace Ona::Engine {
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

		Ona::Core::Chars name;

		uint32_t offset;
	};

	struct MaterialLayout {
		Ona::Core::Slice<Attribute> properties;

		size_t BufferSize() const;

		bool Validate(Ona::Core::Slice<uint8_t const> const & data) const;
	};

	struct VertexLayout {
		uint32_t vertexSize;

		Ona::Core::Slice<Attribute> attributes;

		bool Validate(Ona::Core::Slice<uint8_t const> const & data) const;
	};

	struct GraphicsCommands {

	};

	enum class RendererError {
		Server,
		BadShader
	};

	enum class PolyError {
		Server,
		BadRenderer,
		BadVertices
	};

	enum class MaterialError {
		Server,
		BadRenderer,
		BadData,
		BadShader,
		BadImage
	};

	class GraphicsServer {
		public:
		virtual ~GraphicsServer() { }

		virtual void Clear() = 0;

		virtual void ColoredClear(Ona::Core::Color color) = 0;

		virtual void SubmitCommands(GraphicsCommands const & commands) = 0;

		virtual bool ReadEvents(Events & events) = 0;

		virtual void Update() = 0;

		virtual Ona::Core::Result<ResourceId, RendererError> CreateRenderer(
			Ona::Core::Chars const & vertexSource,
			Ona::Core::Chars const & fragmentSource,
			MaterialLayout const & materialLayout,
			VertexLayout const & vertexLayout
		) = 0;

		virtual Ona::Core::Result<ResourceId, PolyError> CreatePoly(
			ResourceId rendererId,
			Ona::Core::Slice<uint8_t const> const & vertexData
		) = 0;

		virtual Ona::Core::Result<ResourceId, MaterialError> CreateMaterial(
			Ona::Core::Chars const & vertexSource,
			Ona::Core::Chars const & fragmentSource,
			ResourceId rendererId,
			Ona::Core::Slice<uint8_t const> const & materialData,
			Ona::Core::Image const & texture
		) = 0;
	};

	GraphicsServer * LoadOpenGl(Ona::Core::String const & title, int32_t width, int32_t height);

	void UnloadGraphics(GraphicsServer * & graphicsServer);

	Ona::Core::Vector4 NormalizeColor(Ona::Core::Color const & color);

	struct SpriteRenderer {
		ResourceId rendererId;
	};

	Ona::Core::Result<SpriteRenderer, RendererError> CreateSpriteRenderer(
		GraphicsServer * graphicsServer
	);
}

#endif
