#ifndef API_H
#define API_H

#include <stdint.h>
#include <stddef.h>

#ifndef ONA_ENGINE_H

typedef struct Instance Instance;

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

struct OnaContext;

typedef struct {
	uint32_t size;

	void(* init)(void * userdata, struct OnaContext const * ona);

	void(* process)(void * userdata, Events const * events, struct OnaContext const * ona);

	void(* exit)(void * userdata, struct OnaContext const * ona);
} SystemInfo;

typedef struct OnaContext {
	bool(* spawnSystem)(SystemInfo const * info);

	Allocator *(* defaultAllocator)();

	GraphicsQueue *(* graphicsQueueAcquire)();

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

	void(*renderSprite)(
		GraphicsQueue * graphicsQueue,
		Material * spriteMaterial,
		Sprite const * sprite
	);
} OnaContext;

typedef enum {
	KeyA = 0x04,
	KeyB = 0x05,
	KeyC = 0x06,
	KeyD = 0x07,
	KeyE = 0x08,
	KeyF = 0x09,
	KeyG = 0x0a,
	KeyH = 0x0b,
	KeyI = 0x0c,
	KeyJ = 0x0d,
	KeyK = 0x0e,
	KeyL = 0x0f,
	KeyM = 0x10,
	KeyN = 0x11,
	KeyO = 0x12,
	KeyP = 0x13,
	KeyQ = 0x14,
	KeyR = 0x15,
	KeyS = 0x16,
	KeyT = 0x17,
	KeyU = 0x18,
	KeyV = 0x19,
	KeyW = 0x1a,
	KeyX = 0x1b,
	KeyY = 0x1c,
	KeyZ = 0x1d,
} Key;

#ifdef __cplusplus

template<typename Type> SystemInfo const * SystemInfoOf() {
	static SystemInfo const system = {
		.size = sizeof(Type),

		.init = [](void * userdata, OnaContext const * ona) {
			reinterpret_cast<Type *>(userdata)->Init(ona);
		},

		.process = [](void * userdata, Events const * events, OnaContext const * ona) {
			reinterpret_cast<Type *>(userdata)->Process(events, ona);
		},

		.exit = [](void * userdata, OnaContext const * ona) {
			reinterpret_cast<Type *>(userdata)->Exit(ona);
		},
	};

	return &system;
}

template<typename Type> struct Channel {
	private:
	Type value;

	public:
	Type & Await() {
		return this->value;
	}

	void Pass(Type const & value) {
		this->value = value;
	}
};

#endif

#endif
