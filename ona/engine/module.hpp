#ifndef ENGINE_H
#define ENGINE_H

#include "ona/core/module.hpp"
#include "ona/collections/module.hpp"

namespace Ona::Engine {
	using namespace Ona::Collections;
	using namespace Ona::Core;

	using ResourceID = uint32_t;

	struct Events {
		float deltaTime;
	};

	enum class PropertyType : uint8_t {
		Int8,
		Uint8,
		Int16,
		Uint16,
		Int32,
		Uint32,
		Float32,
		Float64
	};

	struct Property {
		PropertyType type;

		uint16_t components;

		String name;
	};

	enum class SpriteError {
		GraphicsServer,
	};

	class GraphicsServer;

	class GraphicsCommands {
		public:
		virtual void Dispatch(GraphicsServer * graphicsServer) = 0;
	};

	class GraphicsServer : public Object {
		public:
		virtual void Clear() = 0;

		virtual void ColoredClear(Color color) = 0;

		virtual bool ReadEvents(Events * events) = 0;

		virtual void Update() = 0;

		virtual ResourceID CreateRenderer(
			Chars const & vertexSource,
			Chars const & fragmentSource,
			Slice<Property const> const & vertexProperties,
			Slice<Property const> const & rendererProperties,
			Slice<Property const> const & materialProperties
		) = 0;

		virtual ResourceID CreatePoly(
			ResourceID rendererID,
			Slice<uint8_t const> const & vertexData
		) = 0;

		virtual ResourceID CreateMaterial(
			ResourceID rendererID,
			Ona::Core::Image const & texture
		) = 0;

		virtual void UpdateMaterialUserdata(
			ResourceID materialID,
			Slice<uint8_t const> const & userdata
		) = 0;

		virtual void UpdateRendererUserdata(
			ResourceID rendererID,
			Slice<uint8_t const> const & userdata
		) = 0;

		virtual void RenderPolyInstanced(
			ResourceID rendererID,
			ResourceID polyID,
			ResourceID materialID,
			size_t count
		) = 0;
	};

	GraphicsServer * LoadOpenGl(String const & title, int32_t width, int32_t height);

	struct Sprite {
		ResourceID polyId;

		ResourceID materialId;

		Vector2 dimensions;

		bool Equals(Sprite const & that) const;

		void Free();

		uint64_t ToHash() const;
	};

	Result<Sprite, SpriteError> CreateSprite(
		GraphicsServer * graphics,
		Image const & image
	);

	class SpriteCommands final : public GraphicsCommands, public Object {
		struct Chunk {
			static constexpr size_t max = 128;

			Matrix projectionTransform;

			Matrix transforms[max];

			Vector4 viewports[max];
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

		HashTable<Sprite, BatchSet> batches;

		public:
		SpriteCommands(GraphicsServer * graphicsServer);

		~SpriteCommands() override;

		void Dispatch(GraphicsServer * graphics) override;

		void Draw(Sprite const & sprite, Vector2 position);
	};
}

#endif
