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

enum Allocator {
	Allocator_Default,
};

struct Channel;

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
	Allocator allocator;

	Color * pixels;

	Point2 dimensions;
};

struct Sprite {
	struct Vector3 origin;

	struct Color tint;
};

struct GraphicsServer;

#endif

struct OnaContext;

struct OnaEvents {
	float deltaTime;

	bool keysHeld[512];
};

typedef void (*OnaSystemInitializer)(void * userdata, struct OnaContext const * ona);

typedef void (*OnaSystemProcessor)(
	void * userdata,
	struct OnaContext const * ona,
	struct OnaEvents const * events
);

typedef void (*OnaSystemFinalizer)(void * userdata, struct OnaContext const * ona);

struct OnaSystemInfo {
	uint32_t size;

	OnaSystemInitializer initializer;

	OnaSystemProcessor processor;

	OnaSystemFinalizer finalizer;
};

struct OnaContext {
	struct GraphicsQueue * (*acquireGraphicsQueue)(struct GraphicsServer * graphicsServer);

	uint32_t (*channelReceive)(
		struct Channel * channel,
		size_t outputBufferLength,
		void * outputBufferPointer
	);

	uint32_t (*channelSend)(
		struct Channel * channel,
		size_t inputBufferLength,
		void const * inputBufferPointer
	);

	void (*freeChannel)(struct Channel * * channel);

	void (*freeImage)(struct Image * imageResult);

	void (*freeMaterial)(struct GraphicsServer * graphicsServer, struct Material * * material);

	bool (*loadImageFile)(struct Stream * stream, Allocator allocator, struct Image * imageResult);

	bool (*loadImageSolid)(
		Allocator allocator,
		struct Point2 dimensions,
		struct Color fillColor,
		struct Image * imageResult
	);

	struct Material * (*loadMaterialImage)(
		struct GraphicsServer * graphicsServer,
		struct Image const * image
	);

	struct GraphicsServer * (*localGraphicsServer)();

	struct Channel * (*openChannel)(uint32_t typeSize);

	void (*renderSprite)(
		struct GraphicsQueue * graphicsQueue,
		struct Material * spriteMaterial,
		struct Sprite const * sprite
	);

	bool (*spawnSystem)(void * module, struct OnaSystemInfo const * systemInfo);

	void (*stringAssign)(String * destinationString, char const * value);

	void (*stringCopy)(String * destinationString, String const * sourceString);

	void (*stringDestroy)(String * string);
};

#endif
