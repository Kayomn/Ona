#include "ona/engine/module.hpp"

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <GL/gl.h>

namespace Ona::Engine {
	using namespace Ona::Core;

	internal size_t CalculatePropertySize(Property const & property) {
		size_t size;

		switch (property.type) {
			case PropertyType::Int8:
			case PropertyType::Uint8: {
				size = 1;
			} break;

			case PropertyType::Int16:
			case PropertyType::Uint16: {
				size = 2;
			} break;

			case PropertyType::Int32:
			case PropertyType::Uint32: {
				size = 4;
			} break;

			case PropertyType::Float32: {
				size = 4;
			} break;

			case PropertyType::Float64: {
				size = 8;
			} break;
		}

		return (size * property.components);
	}

	internal size_t CalculateUserdataSize(Slice<Property const> const & properties) {
		constexpr size_t attributeAlignment = 4;
		size_t size = 0;

		for (auto & property : properties) {
			size_t const propertySize = CalculatePropertySize(property);
			size_t const remainder = (propertySize % attributeAlignment);
			// Avoid branching where possible. This will blast through the loop with a more
			// consistent speed if its just straight arithmetic operations.
			size += (propertySize + ((attributeAlignment - remainder) * (remainder != 0)));
		}

		return size;
	}

	internal size_t CalculateVertexSize(Slice<Property const> const & properties) {
		size_t size = 0;

		for (auto & property : properties) size += CalculatePropertySize(property);

		return size;
	}

	internal bool ValidateUserdata(
		Slice<Property const> const & properties,
		Slice<uint8_t const> const & data
	) {
		size_t const materialSize = CalculateUserdataSize(properties);

		return ((materialSize && (data.length == materialSize) && ((data.length % 4) == 0)));
	}

	internal bool ValidateVertices(
		Slice<Property const> const & properties,
		Slice<uint8_t const> const & data
	) {
		size_t const vertexSize = CalculateVertexSize(properties);

		return (vertexSize && ((data.length % vertexSize) == 0));
	}

	struct Renderer {
		GLuint shaderProgramHandle;

		GLuint userdataBufferHandle;

		Slice<Property const> vertexProperties;

		Slice<Property const> rendererProperties;

		Slice<Property const> materialProperties;
	};

	struct Material {
		ResourceID rendererId;

		GLuint textureHandle;

		GLuint userdataBufferHandle;
	};

	struct Poly {
		ResourceID rendererId;

		GLuint vertexBufferHandle;

		GLuint vertexArrayHandle;

		GLsizei vertexCount;
	};

	constexpr GLuint rendererBufferBindIndex = 0;

	constexpr GLuint materialBufferBindIndex = 1;

	constexpr GLuint materialTextureBindIndex = 0;

	internal GLenum TypeDescriptorToGl(PropertyType typeDescriptor) {
		switch (typeDescriptor) {
			case PropertyType::Int8: return GL_BYTE;
			case PropertyType::Uint8: return GL_UNSIGNED_BYTE;
			case PropertyType::Int16: return GL_SHORT;
			case PropertyType::Uint16: return GL_UNSIGNED_SHORT;
			case PropertyType::Int32: return GL_INT;
			case PropertyType::Uint32: return GL_UNSIGNED_INT;
			case PropertyType::Float32: return GL_FLOAT;
			case PropertyType::Float64: return GL_DOUBLE;
		}
	}

	internal Vector4 NormalizeColor(Color const & color) {
		return Vector4{
			(color.r / (static_cast<float>(0xFF))),
			(color.g / (static_cast<float>(0xFF))),
			(color.b / (static_cast<float>(0xFF))),
			(color.a / (static_cast<float>(0xFF)))
		};
	}

	GraphicsServer * LoadOpenGl(String const & title, int32_t width, int32_t height) {
		using Ona::Collections::Appender;

		thread_local class OpenGlGraphicsServer final : public GraphicsServer {
			Appender<Renderer> renderers;

			Appender<Material> materials;

			Appender<Poly> polys;

			bool UpdateUniformBuffer(
				GLuint uniformBufferHandle,
				size_t uniformBufferSize,
				Slice<uint8_t const> const & data
			) {
				uint8_t * mappedBuffer = static_cast<uint8_t *>(glMapNamedBuffer(
					uniformBufferHandle,
					GL_READ_WRITE
				));

				if (mappedBuffer) {
					CopyMemory(SliceOf(mappedBuffer, uniformBufferSize), data);
					glUnmapNamedBuffer(uniformBufferHandle);

					return true;
				}

				return false;
			}

			GLuint CompileShaderSources(Chars const & vertexSource, Chars const & fragmentSource) {
				static auto compileObject = [](Chars const & source, GLenum shaderType) -> GLuint {
					GLuint const shaderHandle = glCreateShader(shaderType);

					if (shaderHandle && (source.length <= INT32_MAX)) {
						GLint const sourceSize = static_cast<GLint>(source.length);
						GLint isCompiled;

						glShaderSource(shaderHandle, 1, (&source.pointer), (&sourceSize));
						glCompileShader(shaderHandle);
						glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, (&isCompiled));

						if (isCompiled) {
							return shaderHandle;
						} else {
							constexpr size_t errorBufferSize = 1024;
							thread_local GLchar errorBuffer[errorBufferSize] = {};
							GLsizei errorBufferLength;

							glGetShaderInfoLog(
								shaderHandle,
								errorBufferSize,
								(&errorBufferLength),
								errorBuffer
							);

							Ona::Core::OutFile().Write(
								SliceOf(errorBuffer, errorBufferLength).AsBytes()
							);
						}
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
							glDeleteShader(vertexObjectHandle);
							glDeleteShader(fragmentObjectHandle);
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

			Renderer * GetRenderer(ResourceID resourceID) {
				return (&this->renderers.At(resourceID - 1));
			}

			Material * GetMaterial(ResourceID resourceID) {
				return (&this->materials.At(resourceID - 1));
			}

			Poly * GetPoly(ResourceID resourceID) {
				return (&this->polys.At(resourceID - 1));
			}

			public:
			uint64_t timeNow, timeLast;

			SDL_Window * window;

			void * context;

			OpenGlGraphicsServer(Allocator * allocator) :
				renderers{allocator},
				materials{allocator},
				polys{allocator} { }

			~OpenGlGraphicsServer() override {
				this->renderers.ForValues([](Renderer const & renderer) {
					glDeleteProgram(renderer.shaderProgramHandle);
					glDeleteBuffers(1, (&renderer.userdataBufferHandle));
				});

				this->materials.ForValues([](Material const & material) {
					glDeleteTextures(1, (&material.textureHandle));
					glDeleteBuffers(1, (&material.userdataBufferHandle));
				});

				this->polys.ForValues([](Poly const & poly) {
					glDeleteBuffers(1, (&poly.vertexBufferHandle));
					glDeleteVertexArrays(1, (&poly.vertexArrayHandle));
				});

				SDL_GL_DeleteContext(this->context);
				SDL_GL_UnloadLibrary();
				SDL_DestroyWindow(this->window);
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

			bool ReadEvents(Events * events) override {
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

						case SDL_KEYDOWN: {
							events->keysHeld[sdlEvent.key.keysym.scancode] = true;
						} break;

						case SDL_KEYUP: {
							events->keysHeld[sdlEvent.key.keysym.scancode] = false;
						} break;

						default: break;
					}
				}

				return true;
			}

			void Update() override {
				SDL_GL_SwapWindow(this->window);
			}

			ResourceID CreateRenderer(
				Chars const & vertexSource,
				Chars const & fragmentSource,
				Slice<Property const> const & vertexProperties,
				Slice<Property const> const & rendererProperties,
				Slice<Property const> const & materialProperties
			) override {
				size_t const userdataSize = CalculateUserdataSize(rendererProperties);

				if (
					(userdataSize < PTRDIFF_MAX) &&
					(CalculateUserdataSize(materialProperties) < PTRDIFF_MAX)
				) {
					GLuint userdataBufferHandle;

					glCreateBuffers(1, (&userdataBufferHandle));

					// Were the buffers allocated.
					if (glGetError() == GL_NO_ERROR) {
						glNamedBufferData(
							userdataBufferHandle,
							static_cast<GLsizeiptr>(userdataSize),
							nullptr,
							GL_DYNAMIC_DRAW
						);

						// Has the userdata buffer been initialized?
						if (glGetError() == GL_NO_ERROR) {
							GLuint const shaderHandle = this->CompileShaderSources(
								vertexSource,
								fragmentSource
							);

							if (shaderHandle) {
								glUniformBlockBinding(shaderHandle, glGetUniformBlockIndex(
									shaderHandle,
									"Renderer"
								), 0);

								glUniformBlockBinding(shaderHandle, glGetUniformBlockIndex(
									shaderHandle,
									"Material"
								), 1);

								if (this->renderers.Append(Renderer{
									.shaderProgramHandle = shaderHandle,
									.userdataBufferHandle = userdataBufferHandle,
									.vertexProperties = vertexProperties,
									.rendererProperties = rendererProperties,
									.materialProperties = materialProperties
								})) return static_cast<ResourceID>(this->renderers.Count());
							}
						}

						glDeleteBuffers(1, (&userdataBufferHandle));
					}
				}

				return 0;
			}

			ResourceID CreatePoly(
				ResourceID rendererID,
				Slice<uint8_t const> const & vertexData
			) override {
				if (rendererID) {
					Renderer * renderer = this->GetRenderer(rendererID);

					if (ValidateVertices(renderer->vertexProperties, vertexData)) {
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
									glVertexArrayVertexBuffer(
										vertexArrayHandle,
										0,
										vertexBufferHandle,
										0,
										static_cast<GLsizei>(
											CalculateVertexSize(renderer->vertexProperties)
										)
									);

									if (glGetError() == GL_NO_ERROR) {
										GLuint offset = 0;

										for (size_t i = 0; i < renderer->vertexProperties.length; i += 1) {
											size_t const size = CalculatePropertySize(renderer->vertexProperties.At(i));

											if (size >= UINT32_MAX) {
												// Vertex too big for graphics API.
												return 0;
											}

											glEnableVertexArrayAttrib(
												vertexArrayHandle,
												i
											);

											glVertexArrayAttribFormat(
												vertexArrayHandle,
												i,
												renderer->vertexProperties.At(i).components,
												TypeDescriptorToGl(renderer->vertexProperties.At(i).type),
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

										if (this->polys.Append(Poly{
											.rendererId = rendererID,
											.vertexBufferHandle = vertexBufferHandle,
											.vertexArrayHandle = vertexArrayHandle,

											.vertexCount = static_cast<GLsizei>(
												vertexData.length /
												CalculateVertexSize(renderer->vertexProperties)
											)
										})) {
											return static_cast<ResourceID>(this->polys.Count());
										}
									}
								}
							}
						}
					}
				}

				return 0;
			}

			ResourceID CreateMaterial(
				ResourceID rendererID,
				Image const & texture
			) override {
				if (rendererID) {
					Renderer * renderer = this->GetRenderer(rendererID);
					GLuint userdataBufferHandle;

					glCreateBuffers(1, (&userdataBufferHandle));

					if (userdataBufferHandle) {
						GLuint textureHandle;

						glNamedBufferData(
							userdataBufferHandle,
							CalculateUserdataSize(renderer->materialProperties),
							nullptr,
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
								if (texture.pixels.length) {
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

											GLint value;
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

											if (glGetError() != GL_NO_ERROR) return 0;
										}

										if (this->materials.Append(Material{
											.rendererId = rendererID,
											.textureHandle = textureHandle,
											.userdataBufferHandle = userdataBufferHandle
										})) {
											return static_cast<ResourceID>(this->materials.Count());
										}
									}
								}
							}
						}
					}
				}

				return 0;
			}

			void UpdateMaterialUserdata(
				ResourceID materialID,
				Slice<uint8_t const> const & userdata
			) override {
				if (materialID) {
					Material * material = this->GetMaterial(materialID);
					ResourceID const rendererId = material->rendererId;

					if (rendererId) {
						if (ValidateUserdata(this->GetRenderer(rendererId)->materialProperties, userdata)) {
							if (this->UpdateUniformBuffer(
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

			void UpdateRendererUserdata(
				ResourceID rendererID,
				Slice<uint8_t const> const & userdata
			) override {
				if (rendererID) {
					Renderer * renderer = this->GetRenderer(rendererID);

					if (ValidateUserdata(renderer->rendererProperties, userdata)) {
						if (this->UpdateUniformBuffer(
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

			void RenderPolyInstanced(
				ResourceID rendererID,
				ResourceID polyID,
				ResourceID materialID,
				size_t count
			) override {
				if (rendererID && materialID && polyID) {
					if (count <= INT32_MAX) {
						Renderer * renderer = this->GetRenderer(rendererID);
						Material * material = this->GetMaterial(materialID);
						Poly * poly = this->GetPoly(polyID);

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
					}
				}
				// Bad ID.
			}
		} graphicsServer = OpenGlGraphicsServer{DefaultAllocator()};

		constexpr int32_t initFlags = SDL_INIT_EVERYTHING;

		if (SDL_WasInit(initFlags) == initFlags) return &graphicsServer;

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
					glEnable(GL_DEBUG_OUTPUT);
					glEnable(GL_DEPTH_TEST);

					glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) -> void {
						Ona::Core::OutFile().Print(String::From(message));
						Ona::Core::OutFile().Print(String::From("\n"));
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

		return nullptr;
	}
}
