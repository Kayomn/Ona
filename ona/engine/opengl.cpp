#include "ona/engine.hpp"

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <GL/gl.h>

namespace Ona::Engine {
	struct Renderer {
		uint32_t shaderProgramHandle;

		MaterialLayout materialLayout;

		VertexLayout vertexLayout;
	};

	struct Material {
		ResourceId rendererId;

		ResourceId shaderId;

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

		thread_local class OpenGlGraphicsServer extends GraphicsServer {
			public:
			uint64_t timeNow, timeLast;

			SDL_Window * window;

			void * context;

			Appender<Renderer> renderers;

			Appender<Material> materials;

			Appender<Poly> polys;

			~OpenGlGraphicsServer() override {
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
				Ona::Core::Vector4 const rgba = NormalizeColor(color);

				glClearColor(rgba.x, rgba.y, rgba.z, rgba.w);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			}

			void SubmitCommands(GraphicsCommands const & commands) override {

			}

			bool ReadEvents(Events & events) override {
				thread_local SDL_Event sdlEvent;

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

			ResourceId CreateShader(Chars const & sourceCode, ShaderError * error) override {
				return 0;
			}

			ResourceId CreateRenderer(
				ResourceId shaderId,
				MaterialLayout const & materialLayout,
				VertexLayout const & vertexLayout,
				RendererError * error
			) override {
				return 0;
			}

			ResourceId CreatePoly(
				ResourceId rendererId,
				Slice<uint8_t const> const & vertexData,
				PolyError * error
			) override {
				if (rendererId) {
					Renderer * renderer = (&this->renderers.At(rendererId - 1));

					if (renderer->vertexLayout.Validate(vertexData)) {
						GLuint vertexBufferHandle;

						glCreateBuffers(1, &vertexBufferHandle);

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

									glCreateVertexArrays(1, &vertexArrayHandle);

									if (glGetError() == GL_NO_ERROR) {
										glVertexArrayVertexBuffer(
											vertexArrayHandle,
											0,
											vertexBufferHandle,
											0,
											static_cast<GLsizei>(renderer->vertexLayout.vertexSize)
										);

										switch (glGetError()) {
											case GL_NO_ERROR: {
												for (
													auto & attribute :
													renderer->vertexLayout.attributes
												) {
													glEnableVertexArrayAttrib(
														vertexArrayHandle,
														attribute.location
													);

													glVertexArrayAttribFormat(
														vertexArrayHandle,
														attribute.location,
														attribute.components,
														TypeDescriptorToGl(attribute.type),
														false,
														attribute.offset
													);

													glVertexArrayAttribBinding(
														vertexArrayHandle,
														attribute.location,
														0
													);
												}

												ResourceId const id = static_cast<ResourceId>(
													this->polys.Count()
												);

												if (this->polys.Append(Poly{
													vertexBufferHandle,
													vertexArrayHandle
												})) return id;

												if (error) (*error) = PolyError::Server;
											} break;

											case GL_INVALID_VALUE: {
												if (error) (*error) = PolyError::BadVertices;
											} break;

											default: {
												if (error) (*error) = PolyError::Server;
											} break;
										}
									}
								} break;

								case GL_OUT_OF_MEMORY: {
									if (error) (*error) = PolyError::Server;
								} break;

								default: {
									if (error) (*error) = PolyError::BadVertices;
								} break;
							}
						} else if (error) (*error) = PolyError::Server;
					} else if (error) (*error) = PolyError::BadLayout;
				} else if (error) (*error) = PolyError::BadRenderer;

				return 0;
			}

			ResourceId CreateMaterial(
				ResourceId rendererId,
				Slice<uint8_t const> const & materialData,
				Image const & texture,
				ResourceId shaderId,
				MaterialError * error
			) override {
				if (rendererId) {
					if (this->renderers.At(rendererId - 1).materialLayout.Validate(materialData)) {
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
													if (error) (*error) = MaterialError::Server;

													return 0;
												}
											}

											ResourceId const id = static_cast<ResourceId>(
												this->materials.Count()
											);

											if (this->materials.Append(Material{
												rendererId,
												shaderId,
												textureHandle,
												uniformData
											})) return id;
										} else if (error) (*error) = MaterialError::BadImage;
									}
								} break;

								case GL_INVALID_VALUE: {
									if (error) (*error) = MaterialError::BadImage;
								} break;

								default: {
									if (error) (*error) = MaterialError::Server;
								} break;
							}
						} else if (error) (*error) = MaterialError::Server;
					} else if (error) (*error) = MaterialError::BadLayout;
				} else if (error) (*error) = MaterialError::BadRenderer;

				return 0;
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
