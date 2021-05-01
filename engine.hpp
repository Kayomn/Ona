#ifndef ONA_ENGINE_H
#define ONA_ENGINE_H

#include "common.hpp"

#include "engine/image.hpp"

namespace Ona {
	struct Sprite {
		Vector3 origin;

		Color tint;
	};

	struct Material;

	class GraphicsQueue : public Object {
		public:
		virtual void RenderSprite(Material * material, Sprite const & sprite) = 0;
	};

	struct OnaEvents;

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
		 * Acquires a thread-local `GraphicsQueue` instance for dispatching graphics operations
		 * asynchronously.
		 *
		 * The contents of the returned `GraphicsQueue` should be automatically dispatched during
		 * `GraphicsServer::Update`.
		 */
		virtual GraphicsQueue * AcquireQueue() = 0;

		/**
		 * Creates a `Material` from the pixel data in `image`.
		 */
		virtual Material * CreateMaterial(Image const & image) = 0;

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
		virtual bool ReadEvents(OnaEvents * events) = 0;

		/**
		 * Updates the internal state of the `GraphicsServer` and displays the new framebuffer.
		 */
		virtual void Update() = 0;
	};

	GraphicsServer * LoadOpenGL(String title, int32_t width, int32_t height);

	extern GraphicsServer * localGraphicsServer;

	class Module : public Object {
		public:
		virtual void Finalize() = 0;

		virtual void Initialize() = 0;

		virtual void Process(OnaEvents const & events) = 0;
	};

	class Config final : public Object {
		private:
		struct Value;

		HashTable<String, HashTable<String, Value> *> sections;

		public:
		Config(Allocator allocator);

		~Config() override;

		/**
		 * Parses and executes `source` as an Ona configuration file, returning `true` if parsing
		 * was successful, otherwise `false`.
		 *
		 * If `errorMessage` is not null and the returned value is `false`, a human-readable error
		 * message may have been written to `errorMessage`.
		 */
		bool Parse(String const & source, String * errorMessage);

		String ReadString(
			String const & section,
			String const & key,
			String const & fallback
		) const;

		Vector2 ReadVector2(String const & section, String const & key, Vector2 fallback) const;
	};

	#include "api.h"

	class NativeModule : public Module {
		private:
		struct System;

		PackedStack<System> systems;

		void * handle;

		void(*initializer)(OnaContext const * context);

		void(*finalizer)(OnaContext const * context);

		public:
		NativeModule(Allocator allocator, String const & libraryPath);

		~NativeModule() override;

		void Finalize() override;

		void Initialize() override;

		void Process(OnaEvents const & events) override;

		bool SpawnSystem(OnaSystemInfo const & systemInfo);
	};
}

#endif
