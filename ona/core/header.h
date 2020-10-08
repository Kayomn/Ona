#ifndef ONA_CORE_H
#define ONA_CORE_H

#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define internal static

#define thread_local _Thread_local

#define cast(type) (type)

#define min(a, b) ((a < b) ? a : b)

#define make(type) cast(type *)calloc(1, sizeof(type))

typedef void * Allocator;

typedef struct {
	int32_t x, y;
} Point2;

typedef struct {
	float x, y, z, w;
} Vector4;

typedef struct {
	uint8_t r, g, b, a;
} Color;

typedef struct {
	size_t size;

	size_t length;

	char const * pointer;
} Chars;

typedef struct {
	Allocator * allocator;

	Color * pixels;

	Point2 dimensions;
} Image;



#endif
