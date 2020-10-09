#include "ona/engine/header.hpp"

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
		(color.r / (static_cast<float>(0xFF))),
		(color.g / (static_cast<float>(0xFF))),
		(color.b / (static_cast<float>(0xFF))),
		(color.a / (static_cast<float>(0xFF)))
	};
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
		GLint const sourceSize = static_cast<int32_t>(source.length);
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

internal bool updateUniformBuffer(
	GLuint uniformBufferHandle,
	size_t uniformBufferSize,
	DataBuffer data
) {
	uint8_t * mappedBuffer = reinterpret_cast<uint8_t *>(
		glMapNamedBuffer(uniformBufferHandle, GL_READ_WRITE)
	);

	if (mappedBuffer) {
		memcpy(mappedBuffer, data.pointer, uniformBufferSize < data.length ? uniformBufferSize : data.length);
		glUnmapNamedBuffer(uniformBufferHandle);

		return true;
	}

	return false;
}

struct Renderer {
	GLuint shaderProgramHandle;

	GLuint userdataBufferHandle;

	Layout vertexLayout;

	Layout rendererLayout;

	Layout materialLayout;
};

struct Material {
	ResourceId rendererId;

	GLuint textureHandle;

	GLuint userdataBufferHandle;
};

struct Poly {
	ResourceId rendererId;

	GLuint vertexBufferHandle;

	GLuint vertexArrayHandle;

	GLsizei vertexCount;
};

constexpr auto rendererBufferBindIndex = 0;

constexpr auto materialBufferBindIndex = 1;

constexpr auto materialTextureBindIndex = 0;

class OpenGLServer final : public GraphicsServer {
	Renderer renderers[16];

	Material materials[16];

	Poly polys[16];

	size_t rendererCount;

	size_t materialCount;

	size_t polyCount;

	Renderer * getRenderer(ResourceId const id) {
		return (this->renderers + (id - 1));
	}

	Material * getMaterial(ResourceId const id) {
		return (this->materials + (id - 1));
	}

	Poly * getPoly(ResourceId const id) {
		return (this->polys + (id - 1));
	}

	public:
	uint64_t timeNow, timeLast;

	SDL_Window * window;

	void * context;

	void clear() override {
		glClearColor(0.f, 0.f, 0.f, 0.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void coloredClear(Color color) override {
		Vector4 const rgba = normalizeColor(color);

		glClearColor(rgba.x, rgba.y, rgba.z, rgba.w);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	bool readEvents(Events* events) override {
		thread_local SDL_Event sdlEvent;
		this->timeLast = this->timeNow;
		this->timeNow = SDL_GetPerformanceCounter();

		events->deltaTime = (
			(this->timeNow - this->timeLast) *
			(1000 / static_cast<float>(SDL_GetPerformanceFrequency()))
		);

		while (SDL_PollEvent(&sdlEvent)) {
			switch (sdlEvent.type) {
				case SDL_QUIT: return false;

				default: break;
			}
		}

		return true;
	}

	void update() override {
		SDL_GL_SwapWindow(this->window);
	}

	ResourceId requestRenderer(
		Chars const vertexSource,
		Chars const fragmentSource,
		Layout const vertexLayout,
		Layout const rendererLayout,
		Layout const materialLayout
	) override {
		size_t const userdataSize = calculateUserdataSize(rendererLayout);

		if ((userdataSize < PTRDIFF_MAX) && (calculateUserdataSize(materialLayout) < PTRDIFF_MAX)) {
			GLuint userdataBufferHandle;

			glCreateBuffers(1, (&userdataBufferHandle));

			// Were the buffers allocated.
			if (glGetError() == GL_NO_ERROR) {
				glNamedBufferData(
					userdataBufferHandle,
					static_cast<GLsizeiptr>(userdataSize),
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

						assert(this->rendererCount != 16);

						this->renderers[this->rendererCount] = Renderer{
							.shaderProgramHandle = shaderHandle,
							.userdataBufferHandle = userdataBufferHandle,
							.vertexLayout = vertexLayout,
							.rendererLayout = rendererLayout,
							.materialLayout = materialLayout
						};

						this->rendererCount += 1;

						return this->rendererCount;
					}
				}

				glDeleteBuffers(1, (&userdataBufferHandle));
			}
		}

		return 0;
	}

	ResourceId requestMaterial(
		ResourceId rendererId,
		Image texture,
		DataBuffer const userdata
	) override {
		if (rendererId) {
			Renderer * renderer = this->getRenderer(rendererId);
			GLuint userdataBufferHandle;

			glCreateBuffers(1, (&userdataBufferHandle));

			if (userdataBufferHandle) {
				size_t const userdataSize = calculateUserdataSize(renderer->materialLayout);
				GLuint textureHandle;

				glNamedBufferData(
					userdataBufferHandle,
					userdataSize,
					userdata.pointer,
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
								constexpr struct {
									GLenum property;

									GLint value;
								} settings[] = {
									{GL_TEXTURE_MIN_FILTER, GL_LINEAR},
									{GL_TEXTURE_MAG_FILTER, GL_LINEAR},
									{GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE},
									{GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE}
								};

								constexpr auto settingsCount = (
									sizeof(settings) / sizeof(settings[0])
								);

								for (size_t i = 0; i < settingsCount; i += 1) {
									glTextureParameteri(
										textureHandle,
										settings[i].property,
										settings[i].value
									);
								}

								assert(this->materialCount != 16);

								this->materials[this->materialCount] = (Material){
									rendererId,
									textureHandle,
									userdataBufferHandle
								};

								this->materialCount += 1;

								return this->materialCount;
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

	ResourceId requestPoly(ResourceId rendererId, const DataBuffer vertexData) override {
		if (rendererId) {
			Renderer * renderer = this->getRenderer(rendererId);

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
								static_cast<GLsizei>(vertexSize)
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

									offset += static_cast<GLuint>(
										calculateAttributeSize(layout.pointer[i])
									);
								}

								assert(this->polyCount != 16);

								this->polys[this->polyCount] = Poly{
									.rendererId = rendererId,
									.vertexBufferHandle = vertexBufferHandle,
									.vertexArrayHandle = vertexArrayHandle,

									.vertexCount = static_cast<GLsizei>(
										vertexData.length / vertexSize
									)
								};

								this->polyCount += 1;

								return this->polyCount;
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

	void renderPolyInstanced(
		ResourceId rendererId,
		ResourceId polyId,
		ResourceId materialId,
		size_t count
	) override {
		if (rendererId && materialId && polyId) {
			if (count <= INT32_MAX) {
				Renderer * renderer = this->getRenderer(rendererId);
				Material * material = this->getMaterial(materialId);
				Poly * poly = this->getPoly(polyId);

				glBindBufferBase(
					GL_UNIFORM_BUFFER,
					rendererBufferBindIndex,
					renderer->userdataBufferHandle
				);

				glBindBufferBase(
					GL_UNIFORM_BUFFER,
					materialBufferBindIndex,
					material->userdataBufferHandle
				);

				glBindBuffer(GL_ARRAY_BUFFER, poly->vertexBufferHandle);
				glBindVertexArray(poly->vertexArrayHandle);
				glUseProgram(renderer->shaderProgramHandle);
				glBindTextureUnit(materialTextureBindIndex, material->textureHandle);

				glDrawArraysInstanced(
					GL_TRIANGLES,
					0,
					poly->vertexCount,
					static_cast<GLsizei>(count)
				);

				GLenum const err = glGetError();

				if (err != GL_NO_ERROR) {
					printf("error");
				}
			}
		}
		// Bad ID.
	}

	void updateRendererUserdata(ResourceId rendererId, const DataBuffer userdata) override {
		if (rendererId) {
			Renderer * renderer = this->getRenderer(rendererId);

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
};

extern "C" GraphicsServer * loadGraphics(Chars title, int32_t width, int32_t height) {
	static OpenGLServer graphicsServer = {};

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

		graphicsServer.window = SDL_CreateWindow(
			title.pointer,
			WindowPosition,
			WindowPosition,
			width,
			height,
			WindowFlags
		);

		graphicsServer.timeNow = SDL_GetPerformanceCounter();

		if (graphicsServer.window) {
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

			graphicsServer.context = SDL_GL_CreateContext(
				graphicsServer.window
			);

			glewExperimental = true;

			if (graphicsServer.context && (glewInit() == GLEW_OK)) {
				glEnable(GL_DEBUG_OUTPUT);
				glEnable(GL_DEPTH_TEST);

				glDebugMessageCallback([](
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
				}, 0);

				if (glGetError() == GL_NO_ERROR) {
					glViewport(0, 0, width, height);

					if (glGetError() == GL_NO_ERROR) {
						return &graphicsServer;
					}
				}
			}
		}
	}

	return NULL;
}

#endif
