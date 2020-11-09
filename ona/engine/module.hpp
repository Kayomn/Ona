#ifndef ENGINE_H
#define ENGINE_H

#include "ona/core/module.hpp"
#include "ona/collections/module.hpp"

namespace Ona::Engine {
	using namespace Ona::Collections;
	using namespace Ona::Core;

	using ResourceKey = uint32_t;

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

	struct Viewport {
		Point2 size;
	};

	class GraphicsServer : public Object {
		public:
		/**
		 * Clears the backbuffer to black.
		 */
		virtual void Clear() = 0;

		/**
		 * Clears the backbuffer to the color value of `color`.
		 */
		virtual void ColoredClear(Color color) = 0;

		/**
		 * Reads any and all event information known to the `GraphicsServer` at the current frame
		 * and writes it to `events`.
		 *
		 * If the exit signal is received, `false` is returned. Otherwise, `true` is returned to
		 * indicate continued processing.
		 */
		virtual bool ReadEvents(Events * events) = 0;

		/**
		 * Updates the internal state of the `GraphicsServer` and displays the new framebuffer.
		 */
		virtual void Update() = 0;

		virtual Result<ResourceKey> CreateRenderer(
			Chars const & vertexSource,
			Chars const & fragmentSource,
			Slice<Property const> const & vertexProperties,
			Slice<Property const> const & rendererProperties,
			Slice<Property const> const & materialProperties
		) = 0;

		virtual Result<ResourceKey> CreatePoly(
			ResourceKey rendererKey,
			Slice<uint8_t const> const & vertexData
		) = 0;

		virtual Result<ResourceKey> CreateMaterial(
			ResourceKey rendererKey,
			Ona::Core::Image const & texture
		) = 0;

		/**
		 * Registers `dispatcher` as a new event to be called just before displaying the new
		 * framebuffer during calls to `GraphicsServer::Update`.
		 */
		virtual void RegisterDispatcher(Callable<void()> dispatcher) = 0;

		virtual void RenderPolyInstanced(
			ResourceKey rendererKey,
			ResourceKey polyKey,
			ResourceKey materialKey,
			size_t count
		) = 0;

		virtual void UpdateMaterialUserdata(
			ResourceKey materialKey,
			Slice<uint8_t const> const & userdata
		) = 0;

		virtual void UpdateProjection(Matrix const & projectionTransform) = 0;

		virtual void UpdateRendererUserdata(
			ResourceKey rendererKey,
			Slice<uint8_t const> const & userdata
		) = 0;

		virtual Viewport const & ViewportOf() const = 0;
	};

	GraphicsServer * LoadOpenGl(String const & title, int32_t width, int32_t height);

	struct Sprite {
		ResourceKey polyId;

		ResourceKey materialId;

		Vector2 dimensions;

		bool Equals(Sprite const & that) const;

		void Free();

		uint64_t ToHash() const;
	};

	class SpriteRenderer : public Object {
		struct Chunk {
			enum { Max = 128 };

			Matrix transforms[Max];

			Vector4 viewports[Max];
		};

		struct Batch {
			size_t count;

			Chunk chunk;
		};

		Allocator * allocator;

		ResourceKey rendererKey;

		ResourceKey rectPolyKey;

		HashTable<Sprite, PackedStack<Batch> *> batchSets;

		bool isInitialized;

		SpriteRenderer(GraphicsServer * graphicsServer);

		public:
		~SpriteRenderer() override;

		static SpriteRenderer * Acquire(GraphicsServer * graphicsServer);

		Result<Sprite> CreateSprite(GraphicsServer * graphicsServer, Image const & sourceImage);

		void DestroySprite(Sprite const & sprite);

		void Draw(Sprite const & sprite, Vector2 position);

		bool IsInitialized() const override;
	};

	enum class LuaType {
		Nil,
		Number,
		Integer,
		String,
		Function,
		Table,
	};

	struct LuaVar {

	};

	class LuaEngine : public Object {
		public:
		LuaEngine(Allocator * allocator) { }

		~LuaEngine() override;

		String ExecuteSourceFile(Slice<LuaVar> const & returnVars, File const & file);

		String ExecuteSourceScript(Slice<LuaVar> const & returnVars, String const & scriptSource);

		LuaVar GetGlobal(String const & globalName);

		LuaVar GetField(LuaVar const & tableVar, String const & fieldName);

		String VarString(LuaVar const & var);

		LuaType VarType(LuaVar const & var);
	};

	using SystemInitializer = bool(*)(void * userdata, GraphicsServer * graphicsServer);

	using SystemProcessor = void(*)(void * userdata, Events const * events);

	using SystemFinalizer = void(*)(void * userdata);
}

#endif
