#ifndef API_H
#define API_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
	float deltaTime;

	bool keysHeld[512];
} Ona_Events;

struct Ona_CoreContext;

struct Ona_GraphicsContext;

typedef struct Ona_GraphicsQueue Ona_GraphicsQueue;

typedef struct Ona_Material Ona_Material;

typedef struct {
	uint32_t size;

	void(* init)(void * userdata, struct Ona_CoreContext const * core);

	void(* process)(
		void * userdata,
		Ona_Events const * events,
		struct Ona_GraphicsContext const * graphics
	);

	void(* exit)(void * userdata, struct Ona_CoreContext const * core);
} Ona_SystemInfo;

#ifdef __cplusplus

#include "ona/core/module.hpp"

using Ona_Allocator = Ona::Core::Allocator;

using Ona_Point2 = Ona::Core::Point2;

using Ona_Vector2 = Ona::Core::Vector2;

using Ona_Vector3 = Ona::Core::Vector3;

using Ona_Color = Ona::Core::Color;

using Ona_Image = Ona::Core::Image;

#else

typedef struct Ona_Allocator Ona_Allocator;

typedef struct {
	uint32_t x, y;
} Ona_Point2;

typedef struct {
	float x, y;
} Ona_Vector2;

typedef struct {
	float x, y, z;
} Ona_Vector3;

typedef struct {
	uint8_t r, g, b, a;
} Ona_Color;

typedef struct {
	Ona_Allocator * allocator;

	Ona_Color * pixels;

	Ona_Point2 dimensions;
} Ona_Image;

#endif

typedef struct Ona_CoreContext {
	bool(* spawnSystem)(Ona_SystemInfo const * info);

	Ona_Allocator *(* defaultAllocator)();

	Ona_GraphicsQueue *(*graphicsQueueCreate)();

	void(*graphicsQueueFree)(Ona_GraphicsQueue * * allocator);

	bool(* imageSolid)(
		Ona_Allocator * allocator,
		Ona_Point2 dimensions,
		Ona_Color color,
		Ona_Image * imageResult
	);

	void(* imageFree)(Ona_Image * image);

	Ona_Material *(* materialCreate)(Ona_Image const * image);

	void(* materialFree)(Ona_Material * * material);
} Ona_CoreContext;

typedef struct Ona_GraphicsContext {
	void(*renderSprite)(
		Ona_GraphicsQueue * graphicsQueue,
		Ona_Material * spriteMaterial,
		Ona_Vector3 const * position,
		Ona_Color tint
	);
} Ona_GraphicsContext;

typedef enum {
	Ona_A = 0x04,
	Ona_B = 0x05,
	Ona_C = 0x06,
	Ona_D = 0x07,
	Ona_E = 0x08,
	Ona_F = 0x09,
	Ona_G = 0x0a,
	Ona_H = 0x0b,
	Ona_I = 0x0c,
	Ona_J = 0x0d,
	Ona_K = 0x0e,
	Ona_L = 0x0f,
	Ona_M = 0x10,
	Ona_N = 0x11,
	Ona_O = 0x12,
	Ona_P = 0x13,
	Ona_Q = 0x14,
	Ona_R = 0x15,
	Ona_S = 0x16,
	Ona_T = 0x17,
	Ona_U = 0x18,
	Ona_V = 0x19,
	Ona_W = 0x1a,
	Ona_X = 0x1b,
	Ona_Y = 0x1c,
	Ona_Z = 0x1d,
} Ona_Key;

#ifdef __cplusplus

template<typename Type> Ona_SystemInfo const * Ona_SystemInfoOf() {
	static Ona_SystemInfo const system = {
		.size = sizeof(Type),

		.init = [](void * userdata, Ona_CoreContext const * core) {
			reinterpret_cast<Type *>(userdata)->Init(core);
		},

		.process = [](
			void * userdata,
			Ona_Events const * events,
			Ona_GraphicsContext const * graphics
		) {
			reinterpret_cast<Type *>(userdata)->Process(events, graphics);
		},

		.exit = [](void * userdata, Ona_CoreContext const * core) {
			reinterpret_cast<Type *>(userdata)->Exit(core);
		},
	};

	return &system;
}

#endif

#endif
