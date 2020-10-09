#ifndef ONA_ENGINE_H
#define ONA_ENGINE_H

#include "ona/core/header.hpp"

using ResourceId = uint32_t;

struct Events {
	float deltaTime;
};

enum TypeDescriptor {
	Type_Int8,
	Type_Uint8,

	Type_Int16,
	Type_Uint16,

	Type_Int32,
	Type_Uint32,

	Type_Float32,
	Type_Float64,
};

struct Attribute {
	TypeDescriptor descriptor;

	uint32_t components;

	Chars chars;
};

struct Layout {
	size_t length;

	Attribute const * pointer;
};

struct DataBuffer {
	size_t const length;

	uint8_t const * pointer;
};

class GraphicsServer {
	public:
    virtual void clear() = 0;

	virtual void coloredClear(Color color) = 0;

	virtual bool readEvents(Events* events) = 0;

	virtual void update() = 0;

	virtual ResourceId requestRenderer(
		Chars const vertexSource,
		Chars const fragmentSource,
		Layout const vertexLayout,
		Layout const rendererLayout,
		Layout const materialLayout
	) = 0;

	virtual ResourceId requestMaterial(
		ResourceId rendererId,
		Image texture,
		DataBuffer const userdata
	) = 0;

	virtual ResourceId requestPoly(ResourceId rendererId, const DataBuffer vertexData) = 0;

	virtual void renderPolyInstanced(
		ResourceId rendererId,
		ResourceId polyId,
		ResourceId materialId,
		size_t count
	) = 0;

	virtual void updateRendererUserdata(ResourceId rendererId, const DataBuffer userdata) = 0;
};

#endif
