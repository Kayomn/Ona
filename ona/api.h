#ifndef API_H
#define API_H

#include <stdint.h>
#include <stddef.h>

#ifndef ENGINE_H

typedef struct Material Material;

typedef struct Allocator Allocator;

typedef struct GraphicsQueue GraphicsQueue;

typedef struct {
	uint32_t x, y;
} Point2;

typedef struct {
	float x, y;
} Vector2;

typedef struct {
	float x, y, z;
} Vector3;

typedef struct {
	uint8_t r, g, b, a;
} Color;

typedef struct {
	Allocator * allocator;

	Color * pixels;

	Point2 dimensions;
} Image;

typedef struct {
	Vector3 origin;

	Color tint;
} Sprite;

#endif

typedef struct Events {
	float deltaTime;

	bool keysHeld[512];
} Events;

struct Context;

typedef struct {
	uint32_t size;

	void(* init)(void * userdata, struct Context const * ona);

	void(* process)(void * userdata, Events const * events, struct Context const * ona);

	void(* exit)(void * userdata, struct Context const * ona);
} SystemInfo;

typedef struct Context {
	bool(* spawnSystem)(SystemInfo const * info);

	Allocator *(* defaultAllocator)();

	GraphicsQueue *(* graphicsQueueCreate)();

	void(* graphicsQueueFree)(GraphicsQueue * * allocator);

	bool(* imageSolid)(
		Allocator * allocator,
		Point2 dimensions,
		Color color,
		Image * imageResult
	);

	void(* imageFree)(Image * image);

	bool(* imageLoadBitmap)(Allocator * allocator, char const * fileName, Image * result);

	Material *(* materialCreate)(Image const * image);

	void(* materialFree)(Material * * material);

	float(* randomF32)(float min, float max);

	void(*renderSprite)(
		GraphicsQueue * graphicsQueue,
		Material * spriteMaterial,
		Sprite const * sprite
	);
} Context;

typedef enum {
	A = 0x04,
	B = 0x05,
	C = 0x06,
	D = 0x07,
	E = 0x08,
	F = 0x09,
	G = 0x0a,
	H = 0x0b,
	I = 0x0c,
	J = 0x0d,
	K = 0x0e,
	L = 0x0f,
	M = 0x10,
	N = 0x11,
	O = 0x12,
	P = 0x13,
	Q = 0x14,
	R = 0x15,
	S = 0x16,
	T = 0x17,
	U = 0x18,
	V = 0x19,
	W = 0x1a,
	X = 0x1b,
	Y = 0x1c,
	Z = 0x1d,
} Key;

#ifdef __cplusplus

template<typename Type> SystemInfo const * SystemInfoOf() {
	static SystemInfo const system = {
		.size = sizeof(Type),

		.init = [](void * userdata, Context const * ona) {
			reinterpret_cast<Type *>(userdata)->Init(ona);
		},

		.process = [](void * userdata, Events const * events, Context const * ona) {
			reinterpret_cast<Type *>(userdata)->Process(events, ona);
		},

		.exit = [](void * userdata, Context const * ona) {
			reinterpret_cast<Type *>(userdata)->Exit(ona);
		},
	};

	return &system;
}

#endif

#endif
