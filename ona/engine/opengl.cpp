#include "ona/engine.hpp"

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <GL/gl.h>

using Ona::Core::Chars;
using Ona::Core::Color;
using Ona::Core::Image;
using Ona::Core::Result;
using Ona::Core::Slice;
using Ona::Core::String;
using Ona::Core::Vector4;

namespace Ona::Engine {
	struct Renderer {
		GLuint shaderProgramHandle;

		GLuint materialBufferHandle;

		size_t materialBufferSize;

		Layout materialLayout;

		Layout vertexLayout;
	};

	struct Material {
		ResourceId rendererId;

		GLuint shaderProgramHandle;

		GLuint textureHandle;

		Slice<uint8_t> uniformData;
	};

	struct Poly {
		ResourceId rendererId;

		GLuint vertexBufferHandle;

		GLuint vertexArrayHandle;

		uint32_t vertexCount;
	};

	GLenum TypeDescriptorToGl(TypeDescriptor typeDescriptor) {
		switch (typeDescriptor) {
			case TypeDescriptor::Byte: return GL_BYTE;
			case TypeDescriptor::UnsignedByte: return GL_UNSIGNED_BYTE;
			case TypeDescriptor::Short: return GL_SHORT;
			case TypeDescriptor::UnsignedShort: return GL_UNSIGNED_SHORT;
			case TypeDescriptor::Int: return GL_INT;
			case TypeDescriptor::UnsignedInt: return GL_UNSIGNED_INT;
			case TypeDescriptor::Float: return GL_FLOAT;
			case TypeDescriptor::Double: return GL_DOUBLE;
		}
	}

	GraphicsServer * LoadOpenGl(String const & title, int32_t width, int32_t height) {
		using Ona::Collections::Appender;

		thread_local class OpenGlGraphicsServer final extends GraphicsServer {
			Appender<Renderer> renderers;

			Appender<Material> materials;

			Appender<Poly> polys;

			GLuint CompileShaderSources(Chars const & vertexSource, Chars const & fragmentSource) {
				static let compileObject = [](Chars const & source, GLenum shaderType) -> GLuint {
					GLuint const shaderHandle = glCreateShader(shaderType);

					if (shaderHandle && (source.length < INT32_MAX)) {
						GLint const sourceSize = static_cast<GLint>(source.length);
						GLint isCompiled;

						glShaderSource(shaderHandle, 1, (&source.pointer), (&sourceSize));
						glCompileShader(shaderHandle);
						glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, (&isCompiled));

						if (isCompiled) return shaderHandle;
					}

					return 0;
				};

				GLuint const vertexObjectHandle = compileObject(vertexSource, GL_VERTEX_SHADER);

				if (vertexObjectHandle) {
					GLuint const fragmentObjectHandle = compileObject(
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

			public:
			uint64_t timeNow, timeLast;

			SDL_Window * window;

			void * context;

			~OpenGlGraphicsServer() override {
				for (let & renderer : this->renderers.Values()) {
					glDeleteShader(renderer.shaderProgramHandle);
				}

				for (let & material : this->materials.Values()) {
					glDeleteShader(material.shaderProgramHandle);
					glDeleteTextures(1, (&material.textureHandle));
					Ona::Core::Deallocate(material.uniformData.pointer);
				}

				for (let & poly : this->polys.Values()) {
					glDeleteBuffers(1, (&poly.vertexBufferHandle));
					glDeleteVertexArrays(1, (&poly.vertexArrayHandle));
				}

				SDL_GL_DeleteContext(this->context);
				SDL_DestroyWindow(this->window);
				SDL_GL_UnloadLibrary();
				SDL_Quit();
			}

			void Clear() override {
				glClearColor(0.f, 0.f, 0.f, 0.f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			}

			void ColoredClear(Color color) override {
				Vector4 const rgba = NormalizeColor(color);

				glClearColor(rgba.x, rgba.y, rgba.z, rgba.w);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			}

			void SubmitCommands(GraphicsCommands const & commands) override {

			}

			bool ReadEvents(Events & events) override {
				thread_local SDL_Event sdlEvent;
				this->timeLast = this->timeNow;
				this->timeNow = SDL_GetPerformanceCounter();

				events.deltaTime = (
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

			void Update() override {
				SDL_GL_SwapWindow(this->window);
			}

			Result<ResourceId, RendererError> CreateRenderer(
				Chars const & vertexSource,
				Chars const & fragmentSource,
				Layout const & materialLayout,
				Layout const & vertexLayout
			) override {
				using Res = Result<ResourceId, RendererError>;
				size_t const materialSize = materialLayout.MaterialSize();

				if (materialSize < PTRDIFF_MAX) {
					GLuint materialBufferHandle;

					glCreateBuffers(1, (&materialBufferHandle));

					if (glGetError() == GL_NO_ERROR) {
							glNamedBufferData(
							materialBufferHandle,
							static_cast<GLsizeiptr>(materialSize),
							nullptr,
							GL_DYNAMIC_DRAW
						);

						switch (glGetError()) {
							case GL_NO_ERROR: {
								GLuint const shaderHandle = this->CompileShaderSources(
									vertexSource,
									fragmentSource
								);

								if (shaderHandle) {
									ResourceId const id = this->renderers.Count();

									if (this->renderers.Append(Renderer{
										shaderHandle,
										materialBufferHandle,
										materialSize,
										materialLayout,
										vertexLayout
									})) return Res::Ok(id);

									// Out of memory.
									return Res::Fail(RendererError::Server);
								}

								// Failed to compile shader sources into a valid shader.
								return Res::Fail(RendererError::BadShader);
							}

							default: {
								glDeleteBuffers(1, (&materialBufferHandle));

								return Res::Fail(RendererError::Server);
							}
						}
					}

					// Could not create uniform buffer object.
					return Res::Fail(RendererError::Server);
				}

				// Renderer ID is 0.
				return Res::Fail(RendererError::Server);
			}

			Result<ResourceId, PolyError> CreatePoly(
				ResourceId rendererId,
				Slice<uint8_t const> const & vertexData
			) override {
				using Res = Result<ResourceId, PolyError>;

				if (rendererId) {
					Renderer * renderer = (&this->renderers.At(rendererId - 1));

					if (renderer->vertexLayout.ValidateVertexData(vertexData)) {
						GLuint vertexBufferHandle;

						glCreateBuffers(1, (&vertexBufferHandle));

						if (glGetError() == GL_NO_ERROR) {
							glNamedBufferStorage(
								vertexBufferHandle,
								vertexData.length,
								vertexData.pointer,
								GL_DYNAMIC_STORAGE_BIT
							);

							switch (glGetError()) {
								case GL_NO_ERROR: {
									GLuint vertexArrayHandle;

									glCreateVertexArrays(1, (&vertexArrayHandle));

									if (glGetError() == GL_NO_ERROR) {
										glVertexArrayVertexBuffer(
											vertexArrayHandle,
											0,
											vertexBufferHandle,
											0,
											static_cast<GLsizei>(
												renderer->vertexLayout.VertexSize()
											)
										);

										switch (glGetError()) {
											case GL_NO_ERROR: {
												GLuint offset = 0;

												Slice<Attribute const> const attributes =
													renderer->vertexLayout.Attributes();

												for (size_t i = 0; i < attributes.length; i += 1) {
													size_t const size = attributes(i).ByteSize();

													if (size >= UINT32_MAX) {
														// Vertex too big for graphics API.
														return Res::Fail(PolyError::BadVertices);
													}

													glEnableVertexArrayAttrib(
														vertexArrayHandle,
														i
													);

													glVertexArrayAttribFormat(
														vertexArrayHandle,
														i,
														attributes(i).components,
														TypeDescriptorToGl(attributes(i).type),
														false,
														offset
													);

													glVertexArrayAttribBinding(
														vertexArrayHandle,
														i,
														0
													);

													offset += static_cast<GLuint>(size);
												}

												ResourceId const id = static_cast<ResourceId>(
													this->polys.Count()
												);

												if (this->polys.Append(Poly{
													vertexBufferHandle,
													vertexArrayHandle
												})) return Res::Ok(id);

												// Out of memory.
												return Res::Fail(PolyError::Server);
											} break;

											case GL_INVALID_VALUE: {
												return Res::Fail(PolyError::BadVertices);
											}

											default: return Res::Fail(PolyError::Server);
										}
									}

									// Could not create vertex array object.
									return Res::Fail(PolyError::Server);
								} break;

								case GL_OUT_OF_MEMORY: return Res::Fail(PolyError::Server);

								default: return Res::Fail(PolyError::BadVertices);
							}
						}

						// Could not create vertex buffer object.
						return Res::Fail(PolyError::Server);
					}

					// Vertex data is not valid.
					return Res::Fail(PolyError::BadVertices);
				}

				// Renderer ID is 0.
				return Res::Fail(PolyError::BadRenderer);
			}

			Result<ResourceId, MaterialError> CreateMaterial(
				Chars const & vertexSource,
				Chars const & fragmentSource,
				ResourceId rendererId,
				Slice<uint8_t const> const & materialData,
				Image const & texture
			) override {
				using Res = Result<ResourceId, MaterialError>;

				if (rendererId) {
					if (this->renderers.At(
						rendererId - 1
					).materialLayout.ValidateMaterialData(materialData)) {
						Slice<uint8_t> uniformData = Ona::Core::Allocate(materialData.length);

						if (uniformData) {
							GLuint textureHandle;

							Ona::Core::CopyMemory(uniformData, materialData);
							glCreateTextures(GL_TEXTURE_2D, 1, &textureHandle);

							glTextureStorage2D(
								textureHandle,
								1,
								GL_RGBA8,
								texture.dimensions.x,
								texture.dimensions.y
							);

							switch (glGetError()) {
								case GL_NO_ERROR: {
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
											texture.pixels.pointer
										);

										if (glGetError() == GL_NO_ERROR) {
											constexpr struct {
												GLenum property;

												int32_t value;
											} settings[] = {
												{GL_TEXTURE_MIN_FILTER, GL_LINEAR},
												{GL_TEXTURE_MAG_FILTER, GL_LINEAR},
												{GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE},
												{GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE}
											};

											constexpr size_t settingsCount = (
												sizeof(settings) / sizeof(settings[0])
											);

											for (size_t i = 0; i < settingsCount; i += 1) {
												glTextureParameteri(
													textureHandle,
													settings[i].property,
													settings[i].value
												);

												if (glGetError() != GL_NO_ERROR) {
													return Res::Fail(MaterialError::Server);
												}
											}

											GLuint const shaderHandle = this->CompileShaderSources(
												vertexSource,
												fragmentSource
											);

											if (shaderHandle) {
												ResourceId const id = static_cast<ResourceId>(
													this->materials.Count()
												);

												if (this->materials.Append(Material{
													rendererId,
													shaderHandle,
													textureHandle,
													uniformData
												})) return Res::Ok(id);

												// Out of memory.
												return Res::Fail(MaterialError::Server);
											}

											// Could not compile shader sources into a valid shader.
											return Res::Fail(MaterialError::BadShader);
										}

										// Could not create texture from image data.
										return Res::Fail(MaterialError::BadImage);
									}

									// Image data is null.
									return Res::Fail(MaterialError::BadImage);
								}

								case GL_INVALID_VALUE: return Res::Fail(MaterialError::BadImage);

								default: return Res::Fail(MaterialError::Server);
							}
						}

						// Failed to allocate uniform buffer object.
						return Res::Fail(MaterialError::Server);
					}

					// Provided material data is not valid.
					return Res::Fail(MaterialError::BadData);
				}

				// Renderer ID is 0.
				return Res::Fail(MaterialError::BadRenderer);
			}

			void UpdateRendererMaterial(ResourceId rendererId, ResourceId materialId) override {
				if (rendererId) {
					Renderer * renderer = (&this->renderers.At(rendererId - 1));

					if (materialId) {
						Material * material = (&this->materials.At(materialId - 1));

						if (material->rendererId == rendererId) {
							GLuint const bufferHandle = renderer->materialBufferHandle;

							uint8_t * mappedBuffer = static_cast<uint8_t *>(glMapNamedBuffer(
								bufferHandle,
								GL_READ_WRITE
							));

							if (mappedBuffer) {
								CopyMemory(
									Slice<uint8_t>::Of(mappedBuffer, renderer->materialBufferSize),
									material->uniformData
								);

								glUnmapNamedBuffer(bufferHandle);
							}
							// GPU memory buffer failed to map to native memory for a variety of
							// platform-specific reasons.
						}
						// Material cannot be used with this renderer.
					}
					// Material ID is zero.
				}
				// Renderer ID is zero.
			}
		} graphicsServer = {};

		constexpr int32_t initFlags = SDL_INIT_EVERYTHING;

		if (SDL_WasInit(initFlags) == initFlags) {
			return &graphicsServer;
		}

		if (SDL_Init(initFlags) == 0) {
			constexpr int32_t windowPosition = SDL_WINDOWPOS_UNDEFINED;
			constexpr int32_t windowFlags = (SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);

			// Fixes a bug on KDE desktops where launching the process disables the default
			// compositor.
			SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");

			graphicsServer.window = SDL_CreateWindow(
				String::Sentineled(title).AsChars().pointer,
				windowPosition,
				windowPosition,
				width,
				height,
				windowFlags
			);

			graphicsServer.timeNow = SDL_GetPerformanceCounter();

			if (graphicsServer.window) {
				if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4) != 0) return nullptr;

				if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3) != 0) return nullptr;

				if (SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) != 0) return nullptr;

				if (SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24) != 0) return nullptr;

				graphicsServer.context = SDL_GL_CreateContext(graphicsServer.window);
				glewExperimental = true;

				if (graphicsServer.context && (glewInit() == GLEW_OK)) {
					glEnable(GL_DEPTH_TEST);

					if (glGetError() == GL_NO_ERROR) {
						glViewport(0, 0, width, height);

						if (glGetError() == GL_NO_ERROR) {
							return &graphicsServer;
						}
					}
				}
			}
		}

		return nullptr;
	}

	void UnloadGraphics(GraphicsServer * & graphicsServer) {
		if (graphicsServer) {
			graphicsServer->~GraphicsServer();
		}
	}
}
