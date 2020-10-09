#ifndef ONA_CORE_H
#define ONA_CORE_H

#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define internal static

class Allocator;

struct Point2 {
	int32_t x, y;
};

struct Vector4 {
	float x, y, z, w;
};

struct Color {
	uint8_t r, g, b, a;
};

struct Chars {
	size_t size;

	char const * pointer;

	size_t length;
};

struct Image {
	Allocator * allocator;

	Color * pixels;

	Point2 dimensions;
};

extern "C" void __cxa_pure_virtual() {
	assert(false);
}

#endif
