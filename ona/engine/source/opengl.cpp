#include "ona/engine/header.hpp"

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <GL/gl.h>

namespace Ona::Engine {
	using Ona::Core::Chars;
	using Ona::Core::Color;
	using Ona::Core::Image;
	using Ona::Core::Optional;
	using Ona::Core::Result;
	using Ona::Core::Slice;
	using Ona::Core::SliceOf;
	using Ona::Core::String;
	using Ona::Core::Vector4;
	using Ona::Core::nil;

	struct Renderer {
		GLuint shaderProgramHandle;

		GLuint userdataBufferHandle;

		Layout vertexLayout;

		Layout userdataLayout;

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

	constexpr GLuint rendererBufferBindIndex = 0;

	constexpr GLuint materialBufferBindIndex = 1;

	constexpr GLuint materialTextureBindIndex = 0;

	internal GLenum TypeDescriptorToGl(TypeDescriptor typeDescriptor) {
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

	internal Vector4 NormalizeColor(Color const & color) {
		return Vector4{
			(color.r / (static_cast<float>(0xFF))),
			(color.g / (static_cast<float>(0xFF))),
			(color.b / (static_cast<float>(0xFF))),
			(color.a / (static_cast<float>(0xFF)))
		};
	}

	Optional<GraphicsServer *> LoadOpenGl(String const & title, int32_t width, int32_t height) {
		using Ona::Collections::Appender;

		thread_local class OpenGlGraphicsServer final extends GraphicsServer {
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
				static let compileObject = [](Chars const & source, GLenum shaderType) -> GLuint {
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

			Renderer * GetRenderer(ResourceId resourceId) {
				return (&this->renderers.At(resourceId - 1));
			}

			Material * GetMaterial(ResourceId resourceId) {
				return (&this->materials.At(resourceId - 1));
			}

			Poly * GetPoly(ResourceId resourceId) {
				return (&this->polys.At(resourceId - 1));
			}

			public:
			uint64_t timeNow, timeLast;

			SDL_Window * window;

			void * context;

			~OpenGlGraphicsServer() override {
				for (let & renderer : this->renderers.Values()) {
					glDeleteProgram(renderer.shaderProgramHandle);
					glDeleteBuffers(1, (&renderer.userdataBufferHandle));
				}

				for (let & material : this->materials.Values()) {
					glDeleteTextures(1, (&material.textureHandle));
					glDeleteBuffers(1, (&material.userdataBufferHandle));
				}

				for (let & poly : this->polys.Values()) {
					glDeleteBuffers(1, (&poly.vertexBufferHandle));
					glDeleteVertexArrays(1, (&poly.vertexArrayHandle));
				}

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
				Layout const & vertexLayout,
				Layout const & userdataLayout,
				Layout const & materialLayout
			) override {
				using Res = Result<ResourceId, RendererError>;
				size_t const userdataSize = userdataLayout.MaterialSize();
				RendererError error = RendererError::Server;

				if ((userdataSize < PTRDIFF_MAX) && (materialLayout.MaterialSize() < PTRDIFF_MAX)) {
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
									.vertexLayout = vertexLayout,
									.userdataLayout = userdataLayout,
									.materialLayout = materialLayout
								}).HasValue()) {
									return Res::Ok(
										static_cast<ResourceId>(this->renderers.Count())
									);
								}
							}

							error = RendererError::BadShader;
						}

						glDeleteBuffers(1, (&userdataBufferHandle));
					}
				}

				return Res::Fail(error);
			}

			Result<ResourceId, PolyError> CreatePoly(
				ResourceId rendererId,
				Slice<uint8_t const> const & vertexData
			) override {
				using Res = Result<ResourceId, PolyError>;

				if (rendererId) {
					Renderer * renderer = this->GetRenderer(rendererId);

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

												if (this->polys.Append(Poly{
													.rendererId = rendererId,
													.vertexBufferHandle = vertexBufferHandle,
													.vertexArrayHandle = vertexArrayHandle,

													.vertexCount = static_cast<GLsizei>(
														vertexData.length /
														renderer->vertexLayout.VertexSize()
													)
												}).HasValue()) {
													return Res::Ok(
														static_cast<ResourceId>(this->polys.Count())
													);
												}

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
				ResourceId rendererId,
				Image const & texture
			) override {
				using Res = Result<ResourceId, MaterialError>;

				if (rendererId) {
					Renderer * renderer = this->GetRenderer(rendererId);
					GLuint userdataBufferHandle;

					glCreateBuffers(1, (&userdataBufferHandle));

					if (userdataBufferHandle) {
						GLuint textureHandle;

						glNamedBufferData(
							userdataBufferHandle,
							renderer->materialLayout.MaterialSize(),
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

							switch (glGetError()) {
								case GL_NO_ERROR: {
									if (texture.pixels.HasValue()) {
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

												if (glGetError() != GL_NO_ERROR) {
													return Res::Fail(MaterialError::Server);
												}
											}

											if (this->materials.Append(Material{
												rendererId,
												textureHandle,
												userdataBufferHandle
											}).HasValue()) {
												return Res::Ok(static_cast<ResourceId>(
													this->materials.Count()
												));
											}

											// Out of memory.
											return Res::Fail(MaterialError::Server);
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
						// Failed to write to uniform buffer object.
					}

					// Failed to allocate uniform buffer object.
					return Res::Fail(MaterialError::Server);
				}

				// Renderer ID is 0.
				return Res::Fail(MaterialError::BadRenderer);
			}

			void UpdateMaterialUserdata(
				ResourceId materialId,
				Slice<uint8_t const> const & userdata
			) override {
				if (materialId) {
					Material * material = this->GetMaterial(materialId);
					ResourceId const rendererId = material->rendererId;

					if (rendererId) {
						if (this->GetRenderer(
							rendererId
						)->materialLayout.ValidateMaterialData(userdata)) {
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
				ResourceId rendererId,
				Slice<uint8_t const> const & userdata
			) override {
				if (rendererId) {
					Renderer * renderer = this->GetRenderer(rendererId);

					if (renderer->userdataLayout.ValidateMaterialData(userdata)) {
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
				ResourceId rendererId,
				ResourceId polyId,
				ResourceId materialId,
				size_t count
			) override {
				if (rendererId && materialId && polyId) {
					if (count <= INT32_MAX) {
						let renderer = this->GetRenderer(rendererId);
						let material = this->GetMaterial(materialId);
						let poly = this->GetPoly(polyId);

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
		} graphicsServer = {};

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
				if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4) != 0) {
					return nil<GraphicsServer *>;
				}

				if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3) != 0) {
					return nil<GraphicsServer *>;
				}

				if (SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) != 0) {
					return nil<GraphicsServer *>;
				}

				if (SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24) != 0) {
					return nil<GraphicsServer *>;
				}

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

		return nil<GraphicsServer *>;
	}
}
