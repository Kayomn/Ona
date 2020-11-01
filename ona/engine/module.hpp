#ifndef ENGINE_H
#define ENGINE_H

#include "ona/core/module.hpp"
#include "ona/collections/module.hpp"

namespace Ona::Engine {
	using namespace Ona::Collections;
	using namespace Ona::Core;

	using ResourceID = uint32_t;

	enum {
		Key_A = 4,
		Key_B = 5,
		Key_C = 6,
		Key_D = 7,
		Key_E = 8,
		Key_F = 9,
		Key_G = 10,
		Key_H = 11,
		Key_I = 12,
		Key_J = 13,
		Key_K = 14,
		Key_L = 15,
		Key_M = 16,
		Key_N = 17,
		Key_O = 18,
		Key_P = 19,
		Key_Q = 20,
		Key_R = 21,
		Key_S = 22,
		Key_T = 23,
		Key_U = 24,
		Key_V = 25,
		Key_W = 26,
		Key_X = 27,
		Key_Y = 28,
		Key_Z = 29,

		Key_1 = 30,
		Key_2 = 31,
		Key_3 = 32,
		Key_4 = 33,
		Key_5 = 34,
		Key_6 = 35,
		Key_7 = 36,
		Key_8 = 37,
		Key_9 = 38,
		Key_0 = 39,

		Key_Return = 40,
		Key_Escape = 41,
		Key_Backspace = 42,
		Key_Tab = 43,
		Key_Space = 44,
	};

	struct Events {
		using Keys = bool[512];

		float deltaTime;

		Keys keysHeld;
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
			size_t count;

			Chunk chunk;
		};

		HashTable<Sprite, ArrayStack<Batch> *> batchSets;

		public:
		SpriteCommands(GraphicsServer * graphicsServer);

		~SpriteCommands() override;

		void Dispatch(GraphicsServer * graphics) override;

		void Draw(Sprite const & sprite, Vector2 position);
	};
}

#endif
