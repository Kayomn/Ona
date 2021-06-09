#include <GL/glew.h>
#include <GL/gl.h>
#include <SDL2/SDL.h>

enum {
	Batch2DVertexMax = 512,
};

typedef struct {
	float x, y;
} Vector2;

typedef struct {
	float x, y, z, w;
} Vector4;

typedef struct {
	Vector2 origin;

	Vector2 extent;
} Rect;

static char const * batch2DVS =
	"#version 330\n"

	"in vec4 vert;\n"

	"out vec2 uv;\n"

	"void main() {\n"
	"	uv = vert.zw;"
	"	gl_Position = vec4(\n"
	"		(vert.x / 640.0) - 1.0,\n"
	"		1.0 - (vert.y / 360.0),\n"
	"		0.0,\n"
	"		1.0\n"
	"	);\n"
	"}\n";

static char const * batch2DFS =
	"#version 330\n"

	"in vec2 uv;\n"

	"out vec4 color;\n"

	"uniform sampler2D tex;\n"

	"void main() {\n"
		"color = texture(tex, uv);\n"
	"}\n";

static struct {
	SDL_Window * window;

	SDL_GLContext glContext;

	SDL_Event eventCache;

	GLuint batch2DShader;

	GLuint batch2DVao;

	GLuint batch2DVbo;
} graphicsStore = {0};

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

void graphicsClear(SDL_Color clearColor) {
	GLclampf const r = (clearColor.r / ((GLclampf)0xFF));
	GLclampf const g = (clearColor.g / ((GLclampf)0xFF));
	GLclampf const b = (clearColor.b / ((GLclampf)0xFF));

	glClearColor(r, g, b, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void graphicsDispose() {
	SDL_GL_DeleteContext(graphicsStore.glContext);
	SDL_DestroyWindow(graphicsStore.window);
	SDL_Quit();
}

void graphicsRenderSprites(GLuint textureHandle, GLuint instanceCount, Rect const * instanceRects) {
	enum {
		QuadVertCount = 6,
	};

	Vector4 batchBuffer[Batch2DVertexMax] = {};
	GLuint instancesBatched = 0;
	GLuint verticesBatched = 0;

	while (instancesBatched < instanceCount) {
		GLuint const newVerticesBatched = (verticesBatched + QuadVertCount);

		while ((newVerticesBatched <= Batch2DVertexMax) && (instancesBatched < instanceCount)) {
			Rect const rect = instanceRects[instancesBatched];

			Vector2 const rectEnd = {
				rect.origin.x + instanceRects->extent.x,
				rect.origin.y + instanceRects->extent.y
			};

			batchBuffer[verticesBatched] = (Vector4){rectEnd.x, rectEnd.y, 1.f, 1.f};
			batchBuffer[verticesBatched + 1] = (Vector4){rectEnd.x, rect.origin.y, 1.f, 0.f};
			batchBuffer[verticesBatched + 2] = (Vector4){rect.origin.x, rectEnd.y, 0.f, 1.f};
			batchBuffer[verticesBatched + 3] = (Vector4){rectEnd.x, rect.origin.y, 1.f, 0.f};
			batchBuffer[verticesBatched + 4] = (Vector4){rect.origin.x, rect.origin.y, 0.f, 0.f};
			batchBuffer[verticesBatched + 5] = (Vector4){rect.origin.x, rectEnd.y, 0.f, 1.f};
			instancesBatched += 1;
			verticesBatched = newVerticesBatched;
		}

		glNamedBufferSubData(
			graphicsStore.batch2DVbo,
			0,
			sizeof(Vector4) * verticesBatched,
			batchBuffer
		);

		glBindTextureUnit(0, textureHandle);
		glBindBuffer(GL_ARRAY_BUFFER, graphicsStore.batch2DVbo);
		glBindVertexArray(graphicsStore.batch2DVao);
		glUseProgram(graphicsStore.batch2DShader);
		glDrawArrays(GL_TRIANGLES, 0, verticesBatched);
	}
}

char const * graphicsLoad(char const * title, GLint width, GLint height) {
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
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

			graphicsStore.glContext = SDL_GL_CreateContext(graphicsStore.window);
			glewExperimental = GL_TRUE;

			if (graphicsStore.glContext && glewInit() == GLEW_OK) {
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				// glEnable(GL_CULL_FACE);
				glEnable(GL_BLEND);
				glDisable(GL_DEPTH_TEST);
				glDisable(GL_SCISSOR_TEST);
				glEnable(GL_DEBUG_OUTPUT);
				glDebugMessageCallback(&logGL, NULL);
				glViewport(0, 0, width, height);

				glCreateVertexArrays(1, &graphicsStore.batch2DVao);

				// TODO: Tidy up.
				glCreateBuffers(1, &graphicsStore.batch2DVbo);
				glNamedBufferData(graphicsStore.batch2DVbo, sizeof(Vector4) * Batch2DVertexMax, NULL, GL_DYNAMIC_DRAW);
				glVertexArrayVertexBuffer(graphicsStore.batch2DVao, 0, graphicsStore.batch2DVbo, 0, sizeof(Vector4));
				glVertexArrayAttribFormat(graphicsStore.batch2DVao, 0, 4, GL_FLOAT, GL_FALSE, 0);
				glVertexArrayAttribBinding(graphicsStore.batch2DVao, 0, 0);
				glEnableVertexArrayAttrib(graphicsStore.batch2DVao, 0);

				graphicsStore.batch2DShader = getShaderProgramId(batch2DVS, batch2DFS);

				return "SDL2 & OpenGL 4";
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
