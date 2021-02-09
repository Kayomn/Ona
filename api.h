// THIS IS A GENERATED FILE - DO NOT MODIFY!
#ifndef ONA_API_H
#define ONA_API_H

#include <stdint.h>
#include <stdbool.h>

#ifndef ONA_ENGINE_H

enum Key {
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
};

enum ImageError {
	ImageError_None = 0,
	ImageError_UnsupportedFormat = 1,
	ImageError_OutOfMemory = 2,
};

enum ImageLoadError {
	ImageLoadError_None = 0,
	ImageLoadError_FileError = 1,
	ImageLoadError_UnsupportedFormat = 2,
	ImageLoadError_OutOfMemory = 3,
};

struct Allocator;

struct GraphicsQueue;

struct Material;

struct String {
	uint8_t userdata[32];
};

struct Color {
	uint8_t r;

	uint8_t g;

	uint8_t b;

	uint8_t a;
};

struct Point2 {
	int32_t x;

	int32_t y;
};

struct Vector2 {
	float x;

	float y;
};

struct Vector3 {
	float x;

	float y;

	float z;
};

struct Image {
	Allocator * allocator;

	Color * pixels;

	Point2 dimensions;
};

struct Sprite {
	struct Vector3 origin;

	struct Color tint;
};

#endif

struct OnaContext;

struct OnaEvents {
	float deltaTime;

	bool keysHeld[512];
};

typedef void (*SystemInitializer)(void * userdata, struct OnaContext const * ona);

typedef void (*SystemProcessor)(
	void * userdata,
	struct OnaContext const * ona,
	struct OnaEvents const * events
);

typedef void (*SystemFinalizer)(void * userdata, struct OnaContext const * ona);

struct SystemInfo {
	uint32_t size;

	SystemInitializer init;

	SystemProcessor process;

	SystemFinalizer exit;
};

struct OnaContext {
	bool (*spawnSystem)(struct SystemInfo const * systemInfo);

	struct Allocator * (*defaultAllocator)();

	struct GraphicsQueue * (*graphicsQueueAcquire)();

	enum ImageError (*imageSolid)(
		struct Allocator * allocator,
		struct Point2 dimensions,
		struct Color fillColor,
		struct Image * imageResult
	);

	void (*imageFree)(struct Image * imageResult);

	enum ImageLoadError (*imageLoad)(
		struct Allocator * imageAllocator,
		struct String const * filePath,
		struct Image * imageResult
	);

	struct Material * (*materialCreate)(struct Image const * materialImage);

	void (*materialFree)(struct Material * * material);

	void (*renderSprite)(
		struct GraphicsQueue * graphicsQueue,
		struct Material * spriteMaterial,
		struct Sprite const * sprite
	);
};

#endif
