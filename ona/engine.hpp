#ifndef ENGINE_H
#define ENGINE_H

#include "ona/core.hpp"
#include "ona/collections.hpp"

namespace Ona::Engine {
	using Ona::Core::Chars;
	using Ona::Core::Color;
	using Ona::Core::Optional;
	using Ona::Core::Result;
	using Ona::Core::Slice;
	using Ona::Core::Vector4;

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

		Chars name;

		size_t ByteSize() const;
	};

	struct Layout {
		size_t count;

		Attribute const * attributes;

		constexpr Slice<Attribute const> Attributes() const {
			return Ona::Core::SliceOf(this->attributes, this->count);
		}

		size_t MaterialSize() const;

		size_t VertexSize() const;

		bool ValidateMaterialData(Slice<uint8_t const> const & data) const;

		bool ValidateVertexData(Slice<uint8_t const> const & data) const;
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

	class GraphicsServer;

	class GraphicsCommands {
		public:
		virtual ~GraphicsCommands() { }

		virtual void Dispatch(GraphicsServer * graphicsServer) = 0;

		virtual bool Load(GraphicsServer * graphicsServer) = 0;
	};

	class GraphicsServer {
		Ona::Collections::Appender<GraphicsCommands *> commandBuffers;

		public:
		virtual ~GraphicsServer() { }

		virtual void Clear() = 0;

		virtual void ColoredClear(Color color) = 0;

		virtual bool ReadEvents(Events * events) = 0;

		virtual void Update() = 0;

		template<typename Type> Optional<Type *> CreateCommandBuffer() {
			let createdCommandBuffer = Ona::Core::Allocate(sizeof(Type));

			if (createdCommandBuffer.HasValue()) {
				Type * commandBuffer = reinterpret_cast<Type *>(createdCommandBuffer.pointer);
				(*commandBuffer) = Type{};

				if (commandBuffer->Load(this)) {
					this->commandBuffers.Append(commandBuffer);

					return commandBuffer;
				}
			}

			return Ona::Core::nil<Type *>;
		}

		virtual Result<ResourceId, RendererError> CreateRenderer(
			Chars const & vertexSource,
			Chars const & fragmentSource,
			Layout const & vertexLayout,
			Layout const & userdataLayout,
			Layout const & materialLayout
		) = 0;

		virtual Result<ResourceId, PolyError> CreatePoly(
			ResourceId rendererId,
			Slice<uint8_t const> const & vertexData
		) = 0;

		virtual Result<ResourceId, MaterialError> CreateMaterial(
			ResourceId rendererId,
			Ona::Core::Image const & texture
		) = 0;

		virtual void UpdateMaterialUserdata(
			ResourceId materialId,
			Slice<uint8_t const> const & userdata
		) = 0;

		virtual void UpdateRendererUserdata(
			ResourceId rendererId,
			Slice<uint8_t const> const & userdata
		) = 0;

		virtual void UpdateRendererMaterial(ResourceId rendererId, ResourceId materialId) = 0;

		virtual void RenderPolyInstanced(
			ResourceId rendererId,
			ResourceId polyId,
			ResourceId materialId,
			size_t count
		) = 0;
	};

	Optional<GraphicsServer *> LoadOpenGl(
		Ona::Core::String const & title,
		int32_t width,
		int32_t height
	);

	Vector4 NormalizeColor(Color const & color);

	struct Sprite {
		ResourceId polyId;

		ResourceId materialId;

		void Free();

		uint64_t Hash() const;

		constexpr auto operator<=>(Sprite const &) const = default;
	};

	Optional<Sprite> CreateSprite(
		GraphicsServer * graphics,
		Ona::Core::Image const & image
	);

	class SpriteCommands final extends GraphicsCommands {
		struct Chunk {
			static constexpr size_t max = 128;

			Ona::Core::Matrix transforms[max];

			Vector4 viewports[max];
		};

		struct Batch {
			Batch * next;

			size_t count;

			Chunk chunk;
		};

		struct BatchSet {
			Optional<Batch *> current;

			Batch head;
		};

		Ona::Collections::Table<Sprite, BatchSet> batches;

		public:
		void Dispatch(GraphicsServer * graphics) override;

		void Draw(Sprite sprite, Ona::Core::Vector2 position);

		bool Load(GraphicsServer * graphicsServer) override;
	};
}

#endif
