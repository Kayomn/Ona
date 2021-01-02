#ifndef ENGINE_H
#define ENGINE_H

#include "ona/core/module.hpp"
#include "ona/collections/module.hpp"

namespace Ona::Engine {
	using namespace Ona::Collections;
	using namespace Ona::Core;

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

	struct Events;

	struct Sprite {
		Vector3 origin;

		Color tint;
	};

	struct Material;

	class GraphicsQueue : public Object {
		public:
		virtual void RenderSprite(Material * material, Sprite const & sprite) = 0;
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
		 * Creates a `GraphicsQueue` for dispatching graphics commands asynchronously.
		 *
		 * The contents of the returned `GraphicsQueue` should be automatically dispatched during
		 * `GraphicsServer::Update`.
		 */
		virtual GraphicsQueue * CreateQueue() = 0;

		/**
		 * Creates a `Material` from the pixel data in `image`.
		 */
		virtual Material * CreateMaterial(Image const & image) = 0;

		/**
		 * Attempts to delete the `GraphicsQueue` from the server located at `queue`.
		 */
		virtual void DeleteQueue(GraphicsQueue * & queue) = 0;

		/**
		 * Attempts to delete the `Material` from the server located at `material`.
		 */
		virtual void DeleteMaterial(Material * & material) = 0;

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
	};

	GraphicsServer * LoadOpenGl(String const & title, int32_t width, int32_t height);

	#include "ona/api.h"
}

#include "ona/engine/header/config.hpp"

#endif
