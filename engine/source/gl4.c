#include <GL/glew.h>
#include <GL/gl.h>
#include <SDL2/SDL.h>

enum {
	Batch2DVertexMax = 512,
	QuadVertexCount = 6,
};

typedef struct {
	float x, y;
} Vector2;

typedef struct {
	Vector2 xy;

	Vector2 uv;
} Vertex2D;

typedef struct {
	Vector2 origin;

	Vector2 extent;
} Rect;

typedef struct {
	Rect destinationRect;
} Sprite;

static char const * batch2DVS =
	"#version 440\n"

	"in vec2 vertXY;\n"
	"in vec2 vertUV;\n"

	"out vec2 uv;\n"

	"uniform vec2 displaySize;\n"

	"void main() {\n"
	"	uv = vertUV;\n"
	"	gl_Position = vec4(\n"
	"		(vertXY.x / (displaySize.x / 2)) - 1.0,\n"
	"		1.0 - (vertXY.y / (displaySize.y / 2)),\n"
	"		0.0,\n"
	"		1.0\n"
	"	);\n"
	"}\n";

static char const * batch2DFS =
	"#version 440\n"

	"in vec2 uv;\n"

	"out vec4 color;\n"

	"uniform sampler2D albedo;\n"

	"void main() {\n"
		"color = texture(albedo, uv);\n"
	"}\n";

static struct {
	SDL_Window * window;

	SDL_GLContext glContext;

	SDL_Event eventCache;

	GLuint batch2DShader;

	GLuint batch2DVao;

	GLuint batch2DVbo;

	GLuint batch2DAlbedoTextureHandle;

	GLuint batch2DVertexCount;

	Vertex2D batch2DVertices[Batch2DVertexMax];
} graphicsStore = {0};

static void updateDisplaySize(int width, int height) {
	GLfloat const displaySize[2] = {(GLfloat)width, (GLfloat)height};

	glUseProgram(graphicsStore.batch2DShader);
	glUniform2fv(0, 1, displaySize);
}

static GLuint compileShader(const GLchar *source, GLuint shaderType) {
    GLuint shaderHandler;

    shaderHandler = glCreateShader(shaderType);
    glShaderSource(shaderHandler, 1, &source, 0);
    glCompileShader(shaderHandler);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shaderHandler, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shaderHandler, 512, 0, infoLog);
        printf("Error in compilation of shader:\n%s\n", infoLog);
        exit(1);
    };

    return shaderHandler;
}

static GLuint getShaderProgramId(const char *vertexFile, const char *fragmentFile) {
    GLuint programId, vertexHandler, fragmentHandler;

    vertexHandler = compileShader(vertexFile, GL_VERTEX_SHADER);
    fragmentHandler = compileShader(fragmentFile, GL_FRAGMENT_SHADER);

    programId = glCreateProgram();
    glAttachShader(programId, vertexHandler);
    glAttachShader(programId, fragmentHandler);
    glLinkProgram(programId);

    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, 512, 0, infoLog);
        printf("Error in linking of shaders:\n%s\n", infoLog);
        exit(1);
    }

    glDeleteShader(vertexHandler);
    glDeleteShader(fragmentHandler);

    return programId;
}

static void logGL(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	GLchar const * message,
	void const * userParam
) {
	switch (severity) {
		case GL_DEBUG_SEVERITY_LOW: {
			SDL_LogInfo(SDL_LOG_PRIORITY_INFO, "%s", message);
		} break;

		case GL_DEBUG_SEVERITY_MEDIUM: {
			SDL_LogInfo(SDL_LOG_PRIORITY_WARN, "%s", message);
		} break;

		case GL_DEBUG_SEVERITY_HIGH: {
			SDL_LogInfo(SDL_LOG_PRIORITY_ERROR, "%s", message);
		} break;

		default: break;
	}
}

void graphicsRenderClear(SDL_Color clearColor) {
	GLclampf const r = (clearColor.r / ((GLclampf)0xFF));
	GLclampf const g = (clearColor.g / ((GLclampf)0xFF));
	GLclampf const b = (clearColor.b / ((GLclampf)0xFF));

	glClearColor(r, g, b, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void graphicsExit() {
	SDL_GL_DeleteContext(graphicsStore.glContext);
	SDL_DestroyWindow(graphicsStore.window);
	SDL_Quit();
}

void graphicsRenderSprites(
	GLuint textureHandle,
	GLuint64 spriteCount,
	Sprite const * spriteInstances
) {
	// Second check is a sanity check that will likely be optimized out since they're both compile-
	// time constants.
	if (spriteCount && (QuadVertexCount <= Batch2DVertexMax)) {
		Vertex2D vertices[Batch2DVertexMax] = {};
		GLuint spritesDispatched = 0;
		GLuint verticesBatched = 0;

		// Common resources shared between each batch.
		glBindTextureUnit(0, textureHandle);
		glBindBuffer(GL_ARRAY_BUFFER, graphicsStore.batch2DVbo);
		glBindVertexArray(graphicsStore.batch2DVao);
		glUseProgram(graphicsStore.batch2DShader);

		do {
			// Batch, batch, batch...
			while (
				(verticesBatched + QuadVertexCount <= Batch2DVertexMax) &&
				(spritesDispatched < spriteCount)
			) {
				Rect const rect = spriteInstances[spritesDispatched].destinationRect;

				Vector2 const rectEnd = {
					rect.origin.x + rect.extent.x,
					rect.origin.y + rect.extent.y
				};

				Vertex2D * quadVertices = (vertices + verticesBatched);
				quadVertices[0] = (Vertex2D){{rectEnd.x, rectEnd.y}, {1.f, 1.f}};
				quadVertices[1] = (Vertex2D){{rectEnd.x, rect.origin.y}, {1.f, 0.f}};
				quadVertices[2] = (Vertex2D){{rect.origin.x, rectEnd.y}, {0.f, 1.f}};
				quadVertices[3] = (Vertex2D){{rectEnd.x, rect.origin.y}, {1.f, 0.f}};
				quadVertices[4] = (Vertex2D){{rect.origin.x, rect.origin.y}, {0.f, 0.f}};
				quadVertices[5] = (Vertex2D){{rect.origin.x, rectEnd.y}, {0.f, 1.f}};
				spritesDispatched += 1;
				verticesBatched += QuadVertexCount;
			}

			// Either everything has been batched in that one batch buffer or its space was
			// exhausted - either way, time to render.
			glNamedBufferSubData(
				graphicsStore.batch2DVbo,
				0,
				sizeof(Vertex2D) * verticesBatched,
				vertices
			);

			glDrawArrays(GL_TRIANGLES, 0, verticesBatched);

			// Now to find out if there's anything left to render.
			verticesBatched = 0;
		} while (spritesDispatched < spriteCount);
	}
}

char const * graphicsInit(char const * title, GLint width, GLint height) {
	if (SDL_Init(SDL_INIT_EVERYTHING) == 0) {
		enum {
			WindowPosition = SDL_WINDOWPOS_CENTERED,
		};

		// Fixes a bug on KDE desktops where launching the process disables the default compositor.
		SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");

		graphicsStore.window = SDL_CreateWindow(
			title,
			WindowPosition,
			WindowPosition,
			(int)width,
			(int)height,
			SDL_WINDOW_OPENGL
		);

		if (graphicsStore.window) {
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);
			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

			graphicsStore.glContext = SDL_GL_CreateContext(graphicsStore.window);
			glewExperimental = GL_TRUE;

			if (graphicsStore.glContext && glewInit() == GLEW_OK) {
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glEnable(GL_BLEND);
				glDisable(GL_DEPTH_TEST);
				glDisable(GL_SCISSOR_TEST);
				glEnable(GL_DEBUG_OUTPUT);
				glDebugMessageCallback(&logGL, NULL);
				glViewport(0, 0, width, height);

				glCreateVertexArrays(1, &graphicsStore.batch2DVao);

				// TODO: Tidy up.
				glCreateBuffers(1, &graphicsStore.batch2DVbo);

				glNamedBufferData(
					graphicsStore.batch2DVbo,
					sizeof(Vertex2D) * Batch2DVertexMax,
					NULL,
					GL_DYNAMIC_DRAW
				);

				glVertexArrayVertexBuffer(
					graphicsStore.batch2DVao,
					0,
					graphicsStore.batch2DVbo,
					0,
					sizeof(Vertex2D)
				);

				glVertexArrayAttribFormat(graphicsStore.batch2DVao, 0, 2, GL_FLOAT, GL_FALSE, 0);
				glVertexArrayAttribFormat(graphicsStore.batch2DVao, 1, 2, GL_FLOAT, GL_FALSE, 8);
				glVertexArrayAttribBinding(graphicsStore.batch2DVao, 0, 0);
				glVertexArrayAttribBinding(graphicsStore.batch2DVao, 1, 0);
				glEnableVertexArrayAttrib(graphicsStore.batch2DVao, 0);
				glEnableVertexArrayAttrib(graphicsStore.batch2DVao, 1);

				graphicsStore.batch2DShader = getShaderProgramId(batch2DVS, batch2DFS);

				updateDisplaySize(width, height);

				return "OpenGL 4";
			}
		}
	}

	return NULL;
}

void graphicsUnloadTexture(GLuint handle) {
	glDeleteTextures(1, &handle);
}

GLuint graphicsLoadTexture(SDL_Color const * pixels, GLint imageWidth, GLint imageHeight) {
	if ((imageWidth > 0) && (imageHeight > 0)) {
		GLuint textureHandle = 0;

		glCreateTextures(GL_TEXTURE_2D, 1, (&textureHandle));
		glTextureStorage2D(textureHandle, 1, GL_RGBA8, imageWidth, imageHeight);

		// Was the texture allocated and initialized?
		if (glGetError() == GL_NO_ERROR) {
			glTextureSubImage2D(
				textureHandle,
				0,
				0,
				0,
				imageWidth,
				imageHeight,
				GL_RGBA,
				GL_UNSIGNED_BYTE,
				pixels
			);

			// Was the texture pixel data assigned?
			if (glGetError() == GL_NO_ERROR) {
				// Provided all prerequesites are met, these should not fail.
				glTextureParameteri(textureHandle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTextureParameteri(textureHandle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

				return textureHandle;
			}
		}

		graphicsUnloadTexture(textureHandle);
	}

	return 0;
}

GLboolean graphicsPoll() {
	while (SDL_PollEvent(&graphicsStore.eventCache)) {
		switch (graphicsStore.eventCache.type) {
			case SDL_WINDOWEVENT: {
				if (
					graphicsStore.eventCache.window.windowID ==
					SDL_GetWindowID(graphicsStore.window)
				) {
					switch (graphicsStore.eventCache.window.event)  {
						case SDL_WINDOWEVENT_SIZE_CHANGED: {
							updateDisplaySize(
								graphicsStore.eventCache.window.data1,
								graphicsStore.eventCache.window.data2
							);
						} break;
					}
				}
			} break;

			case SDL_QUIT: return GL_FALSE;

			default: break;
		}
	}

	// So that the event loop doesn't thrash the CPU.
	SDL_Delay(1);

	return GL_TRUE;
}

void graphicsPresent() {
	SDL_GL_SwapWindow(graphicsStore.window);
}
