#include "ona/engine/module.hpp"

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <GL/gl.h>

namespace Ona::Engine {
	using namespace Ona::Collections;
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

	struct Renderer {
		GLuint shaderProgramHandle;

		GLuint rendererBufferHandle;

		Slice<Property const> vertexProperties;

		Slice<Property const> rendererProperties;

		Slice<Property const> materialProperties;
	};

	struct Material {
		ResourceKey rendererId;

		GLuint textureHandle;

		GLuint userdataBufferHandle;
	};

	struct Poly {
		ResourceKey rendererId;

		GLuint vertexBufferHandle;

		GLuint vertexArrayHandle;

		GLsizei vertexCount;
	};

	enum {
		ViewportBufferBindIndex,
		RendererBufferBindIndex,
		MaterialBufferBindIndex
	};

	enum {
		AlbedoTextureBindIndex,
	};

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
		thread_local class OpenGlGraphicsServer final : public GraphicsServer {
			Index<Renderer, ResourceKey> renderers;

			Index<Material, ResourceKey> materials;

			Index<Poly, ResourceKey> polys;

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

			public:
			uint64_t timeNow, timeLast;

			SDL_Window * window;

			void * context;

			GLuint viewportBufferHandle;

			Viewport viewport;

			bool isInitialized;

			OpenGlGraphicsServer(
				Allocator * allocator,
				String const & title,
				int32_t const width,
				int32_t const height
			) :
				renderers{allocator},
				materials{allocator},
				polys{allocator},
				isInitialized{}
			{
				enum { InitFlags = SDL_INIT_EVERYTHING };

				if (SDL_Init(InitFlags) == 0) {
					enum {
						WindowPosition = SDL_WINDOWPOS_UNDEFINED,
						WindowFlags = (SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL)
					};

					// Fixes a bug on KDE desktops where launching the process disables the default
					// compositor.
					SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");

					this->window = SDL_CreateWindow(
						String::Sentineled(title).AsChars().pointer,
						WindowPosition,
						WindowPosition,
						width,
						height,
						WindowFlags
					);

					if (this->window) {
						SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
						SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
						SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
						SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

						this->context = SDL_GL_CreateContext(this->window);
						glewExperimental = true;

						if (this->context && (glewInit() == GLEW_OK)) {
							glCreateBuffers(1, (&this->viewportBufferHandle));

							if (glGetError() == GL_NO_ERROR) {
								glNamedBufferData(
									this->viewportBufferHandle,
									sizeof(Matrix),
									nullptr,
									GL_DYNAMIC_DRAW
								);

								if (glGetError() == GL_NO_ERROR) {
									glBindBufferBase(
										GL_UNIFORM_BUFFER,
										ViewportBufferBindIndex,
										this->viewportBufferHandle
									);

									if (glGetError() == GL_NO_ERROR) {
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
										) -> void {
											File outFile = OutFile();

											outFile.Print(String::From(message));
											outFile.Print(String::From("\n"));
										}, 0);

										glViewport(0, 0, width, height);

										this->viewport = Viewport{
											.size = Point2{width, height}
										};

										this->isInitialized = true;
									}
								}
							}
						}
					}
				}
			}

			~OpenGlGraphicsServer() override {
				this->renderers.ForValues([](Renderer const & renderer) {
					glDeleteProgram(renderer.shaderProgramHandle);
					glDeleteBuffers(1, (&renderer.rendererBufferHandle));
				});

				this->materials.ForValues([](Material const & material) {
					glDeleteTextures(1, (&material.textureHandle));
					glDeleteBuffers(1, (&material.userdataBufferHandle));
				});

				this->polys.ForValues([](Poly const & poly) {
					glDeleteBuffers(1, (&poly.vertexBufferHandle));
					glDeleteVertexArrays(1, (&poly.vertexArrayHandle));
				});

				glDeleteBuffers(1, (&this->viewportBufferHandle));
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

			Result<ResourceKey> CreateRenderer(
				Chars const & vertexSource,
				Chars const & fragmentSource,
				Slice<Property const> const & vertexProperties,
				Slice<Property const> const & rendererProperties,
				Slice<Property const> const & materialProperties
			) override {
				size_t const rendererSize = CalculateUserdataSize(rendererProperties);

				// Are the requested resources feasible to allocate?
				if (
					(rendererSize < PTRDIFF_MAX) &&
					(CalculateUserdataSize(materialProperties) < PTRDIFF_MAX)
				) {
					GLuint rendererBufferHandle;

					glCreateBuffers(1, &rendererBufferHandle);

					// Was the buffer allocated?
					if (glGetError() == GL_NO_ERROR) {
						glNamedBufferData(
							rendererBufferHandle,
							static_cast<GLsizeiptr>(rendererSize),
							nullptr,
							GL_DYNAMIC_DRAW
						);

						// Was the buffer initialized?
						if (glGetError() == GL_NO_ERROR) {
							GLuint const shaderHandle = this->CompileShaderSources(
								vertexSource,
								fragmentSource
							);

							// Was the shader allocated and initialized?
							if (shaderHandle) {
								// Provided all prequesites are met, these should not fail.
								glUniformBlockBinding(
									shaderHandle,
									glGetUniformBlockIndex(shaderHandle, "Viewport"),
									ViewportBufferBindIndex
								);

								glUniformBlockBinding(
									shaderHandle,
									glGetUniformBlockIndex(shaderHandle, "Renderer"),
									RendererBufferBindIndex
								);

								glUniformBlockBinding(
									shaderHandle,
									glGetUniformBlockIndex(shaderHandle, "Material"),
									MaterialBufferBindIndex
								);

								return this->renderers.Insert(Renderer{
									.shaderProgramHandle = shaderHandle,
									.rendererBufferHandle = rendererBufferHandle,
									.vertexProperties = vertexProperties,
									.rendererProperties = rendererProperties,
									.materialProperties = materialProperties
								});
							}

							glDeleteProgram(shaderHandle);
						}

						glDeleteBuffers(1, &rendererBufferHandle);
					}
				}

				return Result<ResourceKey>::Fail();
			}

			Result<ResourceKey> CreatePoly(
				ResourceKey rendererKey,
				Slice<uint8_t const> const & vertexData
			) override {
				Renderer * renderer = this->renderers.Lookup(rendererKey);

				if (renderer) {
					GLsizei vertexSize = 0;

					for (auto & property : renderer->vertexProperties) {
						vertexSize += CalculatePropertySize(property);
					}

					if (vertexSize && ((vertexData.length % vertexSize) == 0)) {
						GLuint vertexBufferHandle;

						glCreateBuffers(1, (&vertexBufferHandle));

						// Was the vertex buffer allocated?
						if (glGetError() == GL_NO_ERROR) {
							glNamedBufferStorage(
								vertexBufferHandle,
								vertexData.length,
								vertexData.pointer,
								GL_DYNAMIC_STORAGE_BIT
							);

							// Was the vertex buffer initialized.
							if (glGetError() == GL_NO_ERROR) {
								GLuint vertexArrayHandle;

								glCreateVertexArrays(1, (&vertexArrayHandle));

								// Was the vertex array allocated?
								if (glGetError() == GL_NO_ERROR) {
									glVertexArrayVertexBuffer(
										vertexArrayHandle,
										0,
										vertexBufferHandle,
										0,
										static_cast<GLsizei>(vertexSize)
									);

									// Was the vertex array initialized?
									if (glGetError() == GL_NO_ERROR) {
										GLuint offset = 0;
										bool badVertexProperty = false;

										for (size_t i = 0; i < renderer->vertexProperties.length && !badVertexProperty; i += 1) {
											size_t const size = CalculatePropertySize(renderer->vertexProperties.At(i));

											badVertexProperty |= (size >= UINT32_MAX);

											glEnableVertexArrayAttrib(
												vertexArrayHandle,
												i
											);

											badVertexProperty |= (glGetError() != GL_NO_ERROR);

											glVertexArrayAttribFormat(
												vertexArrayHandle,
												i,
												renderer->vertexProperties.At(i).components,
												TypeDescriptorToGl(renderer->vertexProperties.At(i).type),
												false,
												offset
											);

											badVertexProperty |= (glGetError() != GL_NO_ERROR);

											glVertexArrayAttribBinding(
												vertexArrayHandle,
												i,
												0
											);

											badVertexProperty |= (glGetError() != GL_NO_ERROR);
											offset += static_cast<GLuint>(size);
										}

										if (!badVertexProperty) return this->polys.Insert(Poly{
											.rendererId = rendererKey,
											.vertexBufferHandle = vertexBufferHandle,
											.vertexArrayHandle = vertexArrayHandle,

											.vertexCount = static_cast<GLsizei>(
												vertexData.length / vertexSize
											)
										});
									}
								}
							}
						}
					}
				}

				return Result<ResourceKey>::Fail();
			}

			Result<ResourceKey> CreateMaterial(
				ResourceKey rendererKey,
				Image const & texture
			) override {
				Renderer * renderer = this->renderers.Lookup(rendererKey);

				if (renderer) {
					GLuint userdataBufferHandle;

					glCreateBuffers(1, (&userdataBufferHandle));

					// Was the buffer allocated?
					if (glGetError() == GL_NO_ERROR) {
						glNamedBufferData(
							userdataBufferHandle,
							CalculateUserdataSize(renderer->materialProperties),
							nullptr,
							GL_DYNAMIC_DRAW
						);

						// Was the buffer initialized?
						if (glGetError() == GL_NO_ERROR) {
							GLuint textureHandle;

							glCreateTextures(GL_TEXTURE_2D, 1, (&textureHandle));

							glTextureStorage2D(
								textureHandle,
								1,
								GL_RGBA8,
								texture.dimensions.x,
								texture.dimensions.y
							);

							// Was the texture allocated and initialized?
							if (glGetError() == GL_NO_ERROR) {
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

								// Was the texture pixel data assigned?
								if (glGetError() == GL_NO_ERROR) {
									// Provided all prerequesites are met, these should not fail.
									glTextureParameteri(
										textureHandle,
										GL_TEXTURE_MIN_FILTER,
										GL_LINEAR
									);

									glTextureParameteri(
										textureHandle,
										GL_TEXTURE_MAG_FILTER,
										GL_LINEAR
									);

									glTextureParameteri(
										textureHandle,
										GL_TEXTURE_WRAP_S,
										GL_CLAMP_TO_EDGE
									);

									glTextureParameteri(
										textureHandle,
										GL_TEXTURE_WRAP_T,
										GL_CLAMP_TO_EDGE
									);

									return this->materials.Insert(Material{
										.rendererId = rendererKey,
										.textureHandle = textureHandle,
										.userdataBufferHandle = userdataBufferHandle
									});
								}
							}
						}
					}
				}

				return Result<ResourceKey>::Fail();
			}

			void RenderPolyInstanced(
				ResourceKey rendererKey,
				ResourceKey polyKey,
				ResourceKey materialKey,
				size_t count
			) override {
				Renderer * renderer = this->renderers.Lookup(rendererKey);
				Material * material = this->materials.Lookup(materialKey);
				Poly * poly = this->polys.Lookup(polyKey);

				if (renderer && material && poly) {
					if (count <= INT32_MAX) {
						glBindBufferBase(
							GL_UNIFORM_BUFFER,
							RendererBufferBindIndex,
							renderer->rendererBufferHandle
						);

						glBindBufferBase(
							GL_UNIFORM_BUFFER,
							MaterialBufferBindIndex,
							material->userdataBufferHandle
						);

						glBindBuffer(GL_ARRAY_BUFFER, poly->vertexBufferHandle);
						glBindVertexArray(poly->vertexArrayHandle);
						glUseProgram(renderer->shaderProgramHandle);
						glBindTextureUnit(AlbedoTextureBindIndex, material->textureHandle);

						glDrawArraysInstanced(
							GL_TRIANGLES,
							0,
							poly->vertexCount,
							static_cast<GLsizei>(count)
						);
					}
				}
			}

			void UpdateMaterialUserdata(
				ResourceKey materialKey,
				Slice<uint8_t const> const & userdata
			) override {
				Material * material = this->materials.Lookup(materialKey);

				if (material) {
					Renderer * renderer = this->renderers.Lookup(material->rendererId);

					if (renderer) {
						Slice<Property const> const materialProperties =
								renderer->materialProperties;

						size_t const materialSize = CalculateUserdataSize(materialProperties);

						if (
							materialSize &&
							(userdata.length == materialSize) &&
							((userdata.length % 4) == 0)
						) {
							uint8_t * mappedBuffer = static_cast<uint8_t *>(glMapNamedBuffer(
								material->userdataBufferHandle,
								GL_WRITE_ONLY
							));

							if (mappedBuffer) {
								CopyMemory(SliceOf(mappedBuffer, materialSize), userdata);
								glUnmapNamedBuffer(material->userdataBufferHandle);
							}
						}
					}
				}
			}

			void UpdateProjection(Matrix const & projectionTransform) override {
				uint8_t * mappedBuffer = static_cast<uint8_t *>(glMapNamedBuffer(
					this->viewportBufferHandle,
					GL_WRITE_ONLY
				));

				if (mappedBuffer) {
					CopyMemory(
						SliceOf(mappedBuffer, sizeof(Matrix)),
						AsBytes(projectionTransform)
					);

					glUnmapNamedBuffer(this->viewportBufferHandle);
				}
			}

			void UpdateRendererUserdata(
				ResourceKey rendererKey,
				Slice<uint8_t const> const & userdata
			) override {
				Renderer * renderer = this->renderers.Lookup(rendererKey);

				if (renderer) {
					Slice<Property const> const rendererProperties = renderer->rendererProperties;
					size_t const rendererSize = CalculateUserdataSize(rendererProperties);

					if (
						rendererSize &&
						(userdata.length == rendererSize) &&
						((userdata.length % 4) == 0)
					) {
						uint8_t * mappedBuffer = static_cast<uint8_t *>(glMapNamedBuffer(
							renderer->rendererBufferHandle,
							GL_WRITE_ONLY
						));

						if (mappedBuffer) {
							CopyMemory(SliceOf(mappedBuffer, rendererSize), userdata);
							glUnmapNamedBuffer(renderer->rendererBufferHandle);
						}
					}
				}
			}

			Viewport const & ViewportOf() const override {
				return this->viewport;
			}
		} graphicsServer = OpenGlGraphicsServer{DefaultAllocator(), title, width, height};

		return (graphicsServer.IsInitialized() ? &graphicsServer : nullptr);
	}
}
