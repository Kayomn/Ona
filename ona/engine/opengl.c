#include "ona/engine/header.h"

#ifdef version_OpenGL

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <GL/gl.h>

#define thread_local _Thread_local

#define internal static

#define cast(type) (type)

typedef enum {
	False,
	True,
} Bool;

typedef struct {
	uint64_t timeNow;

	uint64_t timeLast;

	SDL_Window * sdlWindow;

	SDL_GLContext context;
} OpenGL;

internal void openGLLog(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	GLchar const * message,
	void const * userParam
) {
	printf("%s\n", message);
	fflush(stdout);
}

OpenGL * openGLLoad(char const * title, int32_t width, int32_t height) {
	static thread_local OpenGL openGL;

	enum {
		InitFlags = SDL_INIT_EVERYTHING,
		WindowPosition = SDL_WINDOWPOS_UNDEFINED,
		WindowFlags = (SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL)
	};

	if (SDL_WasInit(InitFlags) == InitFlags) return &openGL;

	if (SDL_Init(InitFlags) == 0) {
		// Fixes a bug on KDE desktops where launching the process disables the default
		// compositor.
		SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");

		openGL.sdlWindow = SDL_CreateWindow(
			title,
			WindowPosition,
			WindowPosition,
			width,
			height,
			WindowFlags
		);

		openGL.timeNow = SDL_GetPerformanceCounter();

		if (openGL.sdlWindow) {
			if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4) != 0) {
				return NULL;
			}

			if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3) != 0) {
				return NULL;
			}

			if (SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) != 0) {
				return NULL;
			}

			if (SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24) != 0) {
				return NULL;
			}

			openGL.context = SDL_GL_CreateContext(openGL.sdlWindow);
			glewExperimental = True;

			if (openGL.context && (glewInit() == GLEW_OK)) {
				glEnable(GL_DEBUG_OUTPUT);
				glEnable(GL_DEPTH_TEST);

				glDebugMessageCallback(openGLLog, 0);

				if (glGetError() == GL_NO_ERROR) {
					glViewport(0, 0, width, height);

					if (glGetError() == GL_NO_ERROR) {
						return &openGL;
					}
				}
			}
		}
	}

	return NULL;
}

void openGLClear() {
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void openGLClearColored(float r, float g, float b, float a) {
	glClearColor(r, g, b, a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

Bool openGLReadEvents(OpenGL * openGL, Events * events) {
	static thread_local SDL_Event sdlEvent;
	openGL->timeLast = openGL->timeNow;
	openGL->timeNow = SDL_GetPerformanceCounter();

	events->deltaTime = (
		(openGL->timeNow - openGL->timeLast) *
		(1000 / cast(float)SDL_GetPerformanceFrequency())
	);

	while (SDL_PollEvent(&sdlEvent)) {
		switch (sdlEvent.type) {
			case SDL_QUIT: return False;

			default: break;
		}
	}

	return True;
}

void openGLUpdate(OpenGL * openGL) {
	SDL_GL_SwapWindow(openGL->sdlWindow);
}

#endif
