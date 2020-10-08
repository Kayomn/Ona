#ifndef ONA_ENGINE_H
#define ONA_ENGINE_H

#include "ona/core/header.h"

typedef uint32_t ResourceId;

typedef struct {
	float deltaTime;
} Events;

typedef enum {
	Type_Int8,
	Type_Uint8,

	Type_Int16,
	Type_Uint16,

	Type_Int32,
	Type_Uint32,

	Type_Float32,
	Type_Float64,
} TypeDescriptor;

typedef struct {
	TypeDescriptor descriptor;

	uint32_t components;
} Attribute;

typedef struct {
	size_t length;

	Attribute const * pointer;
} Layout;

typedef struct {
	size_t length;

	uint8_t const * pointer;
} DataBuffer;

struct opaqueInstance;

typedef struct {
	struct opaqueInstance * instance;

	void (*clearer)(struct opaqueInstance * instance);

	void (*coloredClearer)(struct opaqueInstance * instance, Color color);

	bool (*eventsReader)(struct opaqueInstance * instance, Events * events);

	void (*updater)(struct opaqueInstance * instance);

	ResourceId (*rendererRequester)(
		struct opaqueInstance * instance,
		Chars vertexSource,
		Chars fragmentSource,
		Layout vertexLayout,
		Layout userdataLayout,
		Layout materialLayout
	);

	ResourceId (*polyRequester)(
		struct opaqueInstance * instance,
		ResourceId rendererId,
		DataBuffer vertexData
	);

	ResourceId (*materialCreator)(
		struct opaqueInstance * instance,
		ResourceId rendererId,
		Image texture
	);

	void (*instancedPolyRenderer)(
		struct opaqueInstance * instance,
		ResourceId rendererId,
		ResourceId polyId,
		ResourceId materialId,
		size_t count
	);

	void (*materialUserdataUpdater)(
		struct opaqueInstance * instance,
		ResourceId materialId,
		DataBuffer userdata
	);

	void (*rendererUserdataUpdater)(
		struct opaqueInstance * instance,
		ResourceId rendererId,
		DataBuffer userdata
	);
} GraphicsServer;

#endif
