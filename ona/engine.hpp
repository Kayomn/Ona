#ifndef ENGINE_H
#define ENGINE_H

#include "ona/core.hpp"
#include "ona/collections.hpp"

namespace Ona::Engine {
	using ResourceId = uint32_t;

	struct Events {
		float deltaTime;
	};

	enum class TypeDescriptor : uint8_t {
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
		TypeDescriptor type;

		uint16_t components;

		Ona::Core::Chars name;

		size_t ByteSize() const;
	};

	struct Layout {
		size_t count;

		Attribute const * attributes;

		constexpr Ona::Core::Slice<Attribute const> Attributes() const {
			return Ona::Core::SliceOf(this->attributes, this->count);
		}

		size_t MaterialSize() const;

		size_t VertexSize() const;

		bool ValidateMaterialData(Ona::Core::Slice<uint8_t const> const & data) const;

		bool ValidateVertexData(Ona::Core::Slice<uint8_t const> const & data) const;
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
		BadImage
	};

	class GraphicsServer {
		public:
		virtual ~GraphicsServer() { }

		virtual void Clear() = 0;

		virtual void ColoredClear(Ona::Core::Color color) = 0;

		virtual bool ReadEvents(Events * events) = 0;

		virtual void Update() = 0;

		virtual Ona::Core::Result<ResourceId, RendererError> CreateRenderer(
			Ona::Core::Chars const & vertexSource,
			Ona::Core::Chars const & fragmentSource,
			Layout const & vertexLayout,
			Layout const & userdataLayout,
			Layout const & materialLayout
		) = 0;

		virtual Ona::Core::Result<ResourceId, PolyError> CreatePoly(
			ResourceId rendererId,
			Ona::Core::Slice<uint8_t const> const & vertexData
		) = 0;

		virtual Ona::Core::Result<ResourceId, MaterialError> CreateMaterial(
			ResourceId rendererId,
			Ona::Core::Image const & texture
		) = 0;

		virtual void UpdateMaterialUserdata(
			ResourceId materialId,
			Ona::Core::Slice<uint8_t const> const & userdata
		) = 0;

		virtual void UpdateRendererUserdata(
			ResourceId rendererId,
			Ona::Core::Slice<uint8_t const> const & userdata
		) = 0;

		virtual void UpdateRendererMaterial(ResourceId rendererId, ResourceId materialId) = 0;

		virtual void RenderPolyInstanced(
			ResourceId rendererId,
			ResourceId polyId,
			ResourceId materialId,
			size_t count
		) = 0;
	};

	GraphicsServer * LoadOpenGl(Ona::Core::String const & title, int32_t width, int32_t height);

	Ona::Core::Vector4 NormalizeColor(Ona::Core::Color const & color);

	class RendererCommands {
		public:
		virtual ~RendererCommands() { }

		virtual void Dispatch(GraphicsServer * graphicsServer) = 0;
	};

	struct Sprite {
		ResourceId polyId;

		ResourceId materialId;

		void Free();

		uint64_t Hash() const;

		constexpr bool operator==(Sprite const & that) {
			return ((this->polyId == that.polyId) && (this->materialId == that.materialId));
		}

		constexpr bool operator!=(Sprite const & that) {
			return false;
		}
	};

	Ona::Core::Optional<Sprite> CreateSprite(
		GraphicsServer * graphics,
		Ona::Core::Image const & image
	);

	class SpriteRenderCommands final extends RendererCommands {
		struct Chunk {
			static constexpr size_t max = 128;

			Ona::Core::Matrix transforms[max];

			Ona::Core::Vector4 viewports[max];
		};

		struct Batch {
			Batch * next;

			size_t count;

			Chunk chunk;
		};

		struct BatchSet {
			Batch * current;

			Batch head;
		};

		Ona::Collections::Table<Sprite, BatchSet> batches;

		public:
		SpriteRenderCommands() = default;

		SpriteRenderCommands(GraphicsServer * graphics);

		void Dispatch(GraphicsServer * graphics) override;

		void Draw(Sprite sprite, Ona::Core::Vector2 position);
	};
}

#endif
