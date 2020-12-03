#ifndef API_H
#define API_H

#include "stdint.h"
#include "stdbool.h"

typedef struct {
	uint32_t size;

	bool(*init)(void * userdata);

	void(*process)(void * userdata);

	void(*exit)(void * userdata);
} Ona_SystemInfo;

typedef struct {
	bool(*spawnSystem)(Ona_SystemInfo const * info);
} Ona_API;

#endif
