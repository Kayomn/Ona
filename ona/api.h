#ifndef API_H
#define API_H

#ifdef __cplusplus
#include "ona/core/module.hpp"
#else
#include "stdint.h"
#include "stddef.h"
#endif

struct ona_coreContext;

struct ona_graphicsContext;

typedef struct {
	uint32_t size;

	void(* init)(struct ona_coreContext const * core, void * userdata);

	void(* process)(struct ona_graphicsContext const * graphics, void * userdata);

	void(* exit)(struct ona_coreContext const * core, void * userdata);
} Ona_SystemInfo;

#ifdef __cplusplus

using Ona_Allocator = Ona::Core::Allocator;

using Ona_Point2 = Ona::Core::Point2;

using Ona_Vector2 = Ona::Core::Vector2;

using Ona_Vector3 = Ona::Core::Vector3;

using Ona_Color = Ona::Core::Color;

using Ona_Image = Ona::Core::Image;

template<typename Type> Ona_SystemInfo const * Ona_SystemInfoOf() {
	static Ona_SystemInfo const system = {
		.size = sizeof(Type),

		.init = [](struct ona_coreContext const * core, void * userdata) {
			reinterpret_cast<Type *>(userdata)->Init(core);
		},

		.process = [](struct ona_graphicsContext const * core, void * userdata) {
			reinterpret_cast<Type *>(userdata)->Process(core);
		},

		.exit = [](struct ona_coreContext const * core, void * userdata) {
			reinterpret_cast<Type *>(userdata)->Exit(core);
		},
	};

	return &system;
}

#else

typedef struct Ona_Allocator;

typedef struct {
	uint32_t x, y;
} Ona_Point2;

typedef struct {
	uint8_t r, g, b, a;
} Color;

typedef uint32_t Ona_Resource;

#endif

typedef void Ona_Material;

typedef struct ona_graphicsContext {
	void(*renderSprite)(
		Ona_Material * spriteMaterial,
		Ona_Vector3 const * position,
		Ona_Color tint
	);
} Ona_GraphicsContext;

typedef struct ona_coreContext {
	bool(* spawnSystem)(Ona_SystemInfo const * info);

	Ona_Allocator *(* defaultAllocator)();

	bool(* imageSolid)(
		Ona_Allocator * allocator,
		Ona_Point2 dimensions,
		Ona_Color color,
		Ona_Image * imageResult
	);

	void(* imageFree)(Ona_Image * image);

	Ona_Material *(* createMaterial)(Ona_Image const * image);
} Ona_CoreContext;

#endif
