#include "ona/engine/header.h"

#ifdef version_OpenGL

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <GL/gl.h>

internal GLenum typeDescriptorToGl(TypeDescriptor const typeDescriptor) {
	switch (typeDescriptor) {
		case Type_Int8: return GL_BYTE;
		case Type_Uint8: return GL_UNSIGNED_BYTE;
		case Type_Int16: return GL_SHORT;
		case Type_Uint16: return GL_UNSIGNED_SHORT;
		case Type_Int32: return GL_INT;
		case Type_Uint32: return GL_UNSIGNED_INT;
		case Type_Float32: return GL_FLOAT;
		case Type_Float64: return GL_DOUBLE;
	}
}

internal Vector4 normalizeColor(Color const color) {
	return (Vector4){
		(color.r / (cast(float)(0xFF))),
		(color.g / (cast(float)(0xFF))),
		(color.b / (cast(float)(0xFF))),
		(color.a / (cast(float)(0xFF)))
	};
}

typedef struct {
	GLuint shaderProgramHandle;

	GLuint userdataBufferHandle;

	Layout vertexLayout;

	Layout rendererLayout;

	Layout materialLayout;
} Renderer;

typedef struct {
	ResourceId rendererId;

	GLuint textureHandle;

	GLuint userdataBufferHandle;
} Material;

typedef struct {
	ResourceId rendererId;

	GLuint vertexBufferHandle;

	GLuint vertexArrayHandle;

	GLsizei vertexCount;
} Poly;

typedef struct opaqueInstance {
	Renderer renderers[16];

	Material materials[16];

	Poly polys[16];

	size_t rendererCount;

	size_t materialCount;

	size_t polyCount;

	uint64_t timeNow, timeLast;

	SDL_Window * window;

	void * context;
} Instance;

enum {
	RendererBufferBindIndex = 0,
	MaterialBufferBindIndex = 1,
	MaterialTextureBindIndex = 0,
};

internal void clear(Instance * instance) {
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

internal void coloredClear(Instance * instance, Color color) {
	Vector4 const rgba = normalizeColor(color);

	glClearColor(rgba.x, rgba.y, rgba.z, rgba.w);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

internal bool readEvents(Instance * instance, Events * events) {
	static thread_local SDL_Event sdlEvent;
	instance->timeLast = instance->timeNow;
	instance->timeNow = SDL_GetPerformanceCounter();

	events->deltaTime = (
		(instance->timeNow - instance->timeLast) *
		(1000 / cast(float)(SDL_GetPerformanceFrequency()))
	);

	while (SDL_PollEvent(&sdlEvent)) {
		switch (sdlEvent.type) {
			case SDL_QUIT: return false;

			default: break;
		}
	}

	return true;
}

internal void update(Instance* instance) {
	SDL_GL_SwapWindow(instance->window);
}

internal size_t calculateAttributeSize(Attribute const attribute) {
	size_t size = 0;

	switch (attribute.descriptor) {
		case Type_Int8:
		case Type_Uint8: {
			size = 1;
		} break;

		case Type_Int16:
		case Type_Uint16: {
			size = 2;
		} break;

		case Type_Int32:
		case Type_Uint32: {
			size = 4;
		} break;

		case Type_Float32: {
			size = 4;
		} break;

		case Type_Float64: {
			size = 8;
		} break;
	}

	return (size * attribute.components);
}

internal size_t calculateUserdataSize(Layout const layout) {
	enum { AttributeAlignment = 4 };
	size_t size = 0;

	// This needs to be aligned as per the standards that OpenGL and similar APIs conform to.
	for (size_t i = 0; i < layout.length; i += 1) {
		size_t const attributeSize = calculateAttributeSize(layout.pointer[i]);
		size_t const remainder = (attributeSize % AttributeAlignment);
		// Avoid branching where possible. This will blast through the loop with a more consistent
		// speed if its just straight arithmetic operations.
		size += (attributeSize + ((AttributeAlignment - remainder) * (remainder != 0)));
	}

	return size;
}

internal size_t calculateVertexSize(Layout const layout) {
	size_t size = 0;

	for (size_t i = 0; i < layout.length; i += 1) {
		size += calculateAttributeSize(layout.pointer[i]);
	}

	return size;
}

internal bool validateUserdata(Layout const layout, DataBuffer const data) {
	size_t const userdataSize = calculateUserdataSize(layout);

	return ((userdataSize && (data.length == userdataSize) && ((data.length % 4) == 0)));
}

internal bool validateVertices(Layout const layout, DataBuffer const data) {
	size_t const vertexSize = calculateVertexSize(layout);

	return (vertexSize && ((data.length % vertexSize) == 0));
}

internal GLuint compileShaderObject(Chars const source, GLenum const shaderType) {
	GLuint const shaderHandle = glCreateShader(shaderType);

	if (shaderHandle && (source.length <= INT32_MAX)) {
		GLint const sourceSize = cast(int32_t)(source.length);
		GLint isCompiled;

		glShaderSource(shaderHandle, 1, (&source.pointer), (&sourceSize));
		glCompileShader(shaderHandle);
		glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, (&isCompiled));

		if (isCompiled) {
			return shaderHandle;
		} else {
			enum { ErrorBufferSize = 1024 };
			static thread_local GLchar errorBuffer[ErrorBufferSize] = {};
			GLsizei errorBufferLength;

			glGetShaderInfoLog(shaderHandle, ErrorBufferSize, (&errorBufferLength), errorBuffer);
			printf("%.*s\n", errorBufferLength, errorBuffer);
		}
	}

	return 0;
}

internal GLuint buildShaderProgram(Chars const vertexSource, Chars const fragmentSource) {
	GLuint const vertexObjectHandle = compileShaderObject(vertexSource, GL_VERTEX_SHADER);

	if (vertexObjectHandle) {
		GLuint const fragmentObjectHandle = compileShaderObject(
			fragmentSource,
			GL_FRAGMENT_SHADER
		);

		if (fragmentObjectHandle) {
			GLuint const shaderHandle = glCreateProgram();

			if (shaderHandle) {
				GLint success;

				glAttachShader(shaderHandle, vertexObjectHandle);
				glAttachShader(shaderHandle, fragmentObjectHandle);
				glLinkProgram(shaderHandle);
				glDetachShader(shaderHandle, vertexObjectHandle);
				glDetachShader(shaderHandle, fragmentObjectHandle);
				glGetProgramiv(shaderHandle, GL_LINK_STATUS, (&success));

				if (success) {
					glValidateProgram(shaderHandle);
					glGetProgramiv(shaderHandle, GL_LINK_STATUS, (&success));

					if (success) return shaderHandle;

					// Failed to link program.
				}
			}
		}
	}

	return 0;
}

internal Renderer * getRenderer(Instance * instance, ResourceId const id) {
	return (instance->renderers + (id - 1));
}

internal Material * getMaterial(Instance * instance, ResourceId const id) {
	return (instance->materials + (id - 1));
}

internal Poly * getPoly(Instance * instance, ResourceId const id) {
	return (instance->polys + (id - 1));
}

internal ResourceId requestRenderer(
	Instance * instance,
	Chars vertexSource,
	Chars fragmentSource,
	Layout vertexLayout,
	Layout rendererLayout,
	Layout materialLayout
) {
	size_t const userdataSize = calculateUserdataSize(rendererLayout);

	if ((userdataSize < PTRDIFF_MAX) && (calculateUserdataSize(materialLayout) < PTRDIFF_MAX)) {
		GLuint userdataBufferHandle;

		glCreateBuffers(1, (&userdataBufferHandle));

		// Were the buffers allocated.
		if (glGetError() == GL_NO_ERROR) {
			glNamedBufferData(
				userdataBufferHandle,
				cast(GLsizeiptr)(userdataSize),
				NULL,
				GL_DYNAMIC_DRAW
			);

			// Has the userdata buffer been initialized?
			if (glGetError() == GL_NO_ERROR) {
				GLuint const shaderHandle = buildShaderProgram(vertexSource, fragmentSource);

				if (shaderHandle) {
					glUniformBlockBinding(shaderHandle, glGetUniformBlockIndex(
						shaderHandle,
						"Renderer"
					), 0);

					glUniformBlockBinding(shaderHandle, glGetUniformBlockIndex(
						shaderHandle,
						"Material"
					), 1);

					assert(instance->rendererCount != 16);

					instance->renderers[instance->rendererCount] = (Renderer){
						.shaderProgramHandle = shaderHandle,
						.userdataBufferHandle = userdataBufferHandle,
						.vertexLayout = vertexLayout,
						.rendererLayout = rendererLayout,
						.materialLayout = materialLayout
					};

					instance->rendererCount += 1;

					return instance->rendererCount;
				}
			}

			glDeleteBuffers(1, (&userdataBufferHandle));
		}
	}

	return 0;
}

internal ResourceId requestPoly(
	Instance * instance,
	ResourceId rendererId,
	DataBuffer vertexData
) {
	if (rendererId) {
		Renderer * renderer = getRenderer(instance, rendererId);

		if (validateVertices(renderer->vertexLayout, vertexData)) {
			GLuint vertexBufferHandle;

			glCreateBuffers(1, (&vertexBufferHandle));

			if (glGetError() == GL_NO_ERROR) {
				glNamedBufferStorage(
					vertexBufferHandle,
					vertexData.length,
					vertexData.pointer,
					GL_DYNAMIC_STORAGE_BIT
				);

				if (glGetError() == GL_NO_ERROR) {
					GLuint vertexArrayHandle;

					glCreateVertexArrays(1, (&vertexArrayHandle));

					if (glGetError() == GL_NO_ERROR) {
						size_t const vertexSize = calculateVertexSize(renderer->vertexLayout);

						glVertexArrayVertexBuffer(
							vertexArrayHandle,
							0,
							vertexBufferHandle,
							0,
							cast(GLsizei)vertexSize
						);

						if (glGetError() == GL_NO_ERROR) {
							GLuint offset = 0;
							Layout const layout = renderer->vertexLayout;

							for (size_t i = 0; i < layout.length; i += 1) {
								glEnableVertexArrayAttrib(vertexArrayHandle, i);

								glVertexArrayAttribFormat(
									vertexArrayHandle,
									i,
									layout.pointer[i].components,
									typeDescriptorToGl(layout.pointer[i].descriptor),
									false,
									offset
								);

								glVertexArrayAttribBinding(vertexArrayHandle, i, 0);

								offset += cast(GLuint)calculateAttributeSize(layout.pointer[i]);
							}

							assert(instance->polyCount != 16);

							instance->polys[instance->polyCount] = (Poly){
								.rendererId = rendererId,
								.vertexBufferHandle = vertexBufferHandle,
								.vertexArrayHandle = vertexArrayHandle,
								.vertexCount = cast(GLsizei)(vertexData.length / vertexSize)
							};

							instance->polyCount += 1;

							return instance->polyCount;
						}
					}
				}
			}
			// Could not create vertex buffer object.
		}
		// Vertex data is not valid.
	}
	// Renderer ID is 0.
	return 0;
}

internal ResourceId createMaterial(Instance * instance, ResourceId rendererId, Image texture) {
	if (rendererId) {
		Renderer * renderer = getRenderer(instance, rendererId);
		GLuint userdataBufferHandle;

		glCreateBuffers(1, (&userdataBufferHandle));

		if (userdataBufferHandle) {
			GLuint textureHandle;

			glNamedBufferData(
				userdataBufferHandle,
				calculateUserdataSize(renderer->materialLayout),
				NULL,
				GL_DYNAMIC_DRAW
			);

			if (glGetError() == GL_NO_ERROR) {
				glCreateTextures(GL_TEXTURE_2D, 1, (&textureHandle));

				glTextureStorage2D(
					textureHandle,
					1,
					GL_RGBA8,
					texture.dimensions.x,
					texture.dimensions.y
				);

				if (glGetError() == GL_NO_ERROR) {
					if (texture.pixels) {
						glTextureSubImage2D(
							textureHandle,
							0,
							0,
							0,
							texture.dimensions.x,
							texture.dimensions.y,
							GL_RGBA,
							GL_UNSIGNED_BYTE,
							texture.pixels
						);

						if (glGetError() == GL_NO_ERROR) {
							struct {
								GLenum property;

								GLint value;
							} const settings[] = {
								{GL_TEXTURE_MIN_FILTER, GL_LINEAR},
								{GL_TEXTURE_MAG_FILTER, GL_LINEAR},
								{GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE},
								{GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE}
							};

							enum { SettingsCount = (sizeof(settings) / sizeof(settings[0])) };

							for (size_t i = 0; i < SettingsCount; i += 1) {
								glTextureParameteri(
									textureHandle,
									settings[i].property,
									settings[i].value
								);
							}

							assert(instance->materialCount != 16);

							instance->materials[instance->materialCount] = (Material){
								rendererId,
								textureHandle,
								userdataBufferHandle
							};

							instance->materialCount += 1;

							return instance->materialCount;
						}
					}
				}
			}
			// Failed to write to uniform buffer object.
		}
		// Failed to allocate uniform buffer object.
	}

	// Renderer ID is 0.
	return 0;
}

internal void renderPolyInstanced(
	Instance * instance,
	ResourceId rendererId,
	ResourceId polyId,
	ResourceId materialId,
	size_t count
) {
	if (rendererId && materialId && polyId) {
		if (count <= INT32_MAX) {
			Renderer * renderer = getRenderer(instance, rendererId);
			Material * material = getMaterial(instance, materialId);
			Poly * poly = getPoly(instance, polyId);

			glBindBufferBase(
				GL_UNIFORM_BUFFER,
				RendererBufferBindIndex,
				renderer->userdataBufferHandle
			);

			glBindBufferBase(
				GL_UNIFORM_BUFFER,
				MaterialBufferBindIndex,
				material->userdataBufferHandle
			);

			glBindBuffer(GL_ARRAY_BUFFER, poly->vertexBufferHandle);
			glBindVertexArray(poly->vertexArrayHandle);
			glUseProgram(renderer->shaderProgramHandle);
			glBindTextureUnit(MaterialTextureBindIndex, material->textureHandle);

			glDrawArraysInstanced(
				GL_TRIANGLES,
				0,
				poly->vertexCount,
				cast(GLsizei)(count)
			);

			GLenum const err = glGetError();

			if (err != GL_NO_ERROR) {
				printf("error");
			}
		}
	}
	// Bad ID.
}

internal bool updateUniformBuffer(
	GLuint uniformBufferHandle,
	size_t uniformBufferSize,
	DataBuffer data
) {
	uint8_t * mappedBuffer = cast(uint8_t *)(glMapNamedBuffer(uniformBufferHandle, GL_READ_WRITE));

	if (mappedBuffer) {
		memcpy(mappedBuffer, data.pointer, min(uniformBufferSize, data.length));
		glUnmapNamedBuffer(uniformBufferHandle);

		return true;
	}

	return false;
}

internal void updateMaterialUserdata(
	Instance * instance,
	ResourceId materialId,
	DataBuffer userdata
) {
	if (materialId) {
		Material * material = getMaterial(instance, materialId);
		ResourceId const rendererId = material->rendererId;

		if (rendererId) {
			if (validateUserdata(getRenderer(instance, rendererId)->materialLayout, userdata)) {
				if (updateUniformBuffer(
					material->userdataBufferHandle,
					userdata.length,
					userdata
				)) {
					// TODO: Error codes?
				}
				// GPU memory buffer failed to map to native memory for a variety of
				// platform-specific reasons.
			}
			// Invalid material data.
		}
		// Renderer ID is zero.
	}
	// Material ID is zero.
}

internal void updateRendererUserdata(
	Instance * instance,
	ResourceId rendererId,
	DataBuffer userdata
) {
	if (rendererId) {
		Renderer * renderer = getRenderer(instance, rendererId);

		if (validateUserdata(renderer->rendererLayout, userdata)) {
			if (updateUniformBuffer(
				renderer->userdataBufferHandle,
				userdata.length,
				userdata
			)) {
				// TODO: Error codes?
			}
			// GPU memory buffer failed to map to native memory for a variety of
			// platform-specific reasons.
		}
		// Invalid material data.
	}
	// Renderer ID is zero.
}

internal void logger(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	printf("%s\n", message);
	fflush(stdout);
}

GraphicsServer * loadGraphics(Chars title, int32_t width, int32_t height) {
	static GraphicsServer graphicsServer = {
		.updater = update,
		.clearer = clear,
		.eventsReader = readEvents,
		.coloredClearer = coloredClear,
		.rendererRequester = requestRenderer,
		.materialCreator = createMaterial,
		.polyRequester = requestPoly,
		.materialUserdataUpdater = updateMaterialUserdata,
		.rendererUserdataUpdater = updateRendererUserdata,
		.instancedPolyRenderer = renderPolyInstanced
	};

	enum {
		InitFlags = SDL_INIT_EVERYTHING,
		WindowPosition = SDL_WINDOWPOS_UNDEFINED,
		WindowFlags = (SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL)
	};

	if (SDL_WasInit(InitFlags) == InitFlags) return &graphicsServer;

	if (SDL_Init(InitFlags) == 0) {
		// Fixes a bug on KDE desktops where launching the process disables the default
		// compositor.
		SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");

		graphicsServer.instance = make(Instance);

		if (graphicsServer.instance) {
			graphicsServer.instance->window = SDL_CreateWindow(
				title.pointer,
				WindowPosition,
				WindowPosition,
				width,
				height,
				WindowFlags
			);

			graphicsServer.instance->timeNow = SDL_GetPerformanceCounter();

			if (graphicsServer.instance->window) {
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

				graphicsServer.instance->context = SDL_GL_CreateContext(
					graphicsServer.instance->window
				);

				glewExperimental = true;

				if (graphicsServer.instance->context && (glewInit() == GLEW_OK)) {
					glEnable(GL_DEBUG_OUTPUT);
					glEnable(GL_DEPTH_TEST);

					glDebugMessageCallback(logger, 0);

					if (glGetError() == GL_NO_ERROR) {
						glViewport(0, 0, width, height);

						if (glGetError() == GL_NO_ERROR) {
							return &graphicsServer;
						}
					}
				}
			}
		}
	}

	return NULL;
}

#endif
