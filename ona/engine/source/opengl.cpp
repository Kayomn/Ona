#include "ona/engine/module.hpp"

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <GL/gl.h>

namespace Ona::Engine {
	using namespace Ona::Collections;
	using namespace Ona::Core;

	// (xy, uv) format.
	static Vector4 const quadVertices[] = {
		Vector4{1.f, 1.f, 1.f, 1.f},
		Vector4{1.f, 0.f, 1.f, 0.f},
		Vector4{0.f, 1.f, 0.f, 1.f},
		Vector4{1.f, 0.f, 1.f, 0.f},
		Vector4{0.f, 0.f, 0.f, 0.f},
		Vector4{0.f, 1.f, 0.f, 1.f}
	};

	static Chars const canvasVertexSource = CharsFrom(
		"#version 430 core\n"
		"#define INSTANCE_COUNT 128\n"
		"\n"
		"in vec4 vertex;\n"
		"\n"
		"out vec2 texCoords;\n"
		"out vec4 texTint;\n"
		"\n"
		"layout(std140, row_major) uniform Viewport {\n"
		"	mat4x4 projectionTransform;\n"
		"};\n"
		"\n"
		"layout(std140, row_major) uniform Renderdata {\n"
		"	mat4x4 transforms[INSTANCE_COUNT];\n"
		"	vec4 viewports[INSTANCE_COUNT];\n"
		"	vec4 tints[INSTANCE_COUNT];\n"
		"};\n"
		"\n"
		"uniform sampler2D spriteTexture;\n"
		"\n"
		"void main() {\n"
		"	const vec4 viewport = viewports[gl_InstanceID];\n"
		"	texTint = tints[gl_InstanceID];\n"
		"	texCoords = ((vertex.zw * viewport.zw) + viewport.xy);\n"
		"\n"
		"	gl_Position = (\n"
		"		projectionTransform * transforms[gl_InstanceID] * vec4(vertex.xy, 0.0, 1.0)"
		"	);\n"
		"}\n"
	);

	static Chars const canvasFragmentSource = CharsFrom(
		"#version 430 core\n"
		"\n"
		"in vec2 texCoords;\n"
		"in vec4 texTint;\n"
		"out vec4 outColor;\n"
		"\n"
		"uniform sampler2D spriteTexture;\n"
		"\n"
		"void main() {\n"
		"	const vec4 spriteTextureColor = (texture(spriteTexture, texCoords) * texTint);\n"
		"\n"
		"	if (spriteTextureColor.a == 0.0) discard;\n"
		"\n"
		"	outColor = spriteTextureColor;\n"
		"}\n"
	);

	struct Material {
		Point2 dimensions;

		GLuint textureHandle;
	};

	struct GLPolyBuffer {
		GLuint bufferHandle;

		GLuint arrayHandle;

		GLuint vertexCount;

		bool Load(Slice<Vector4 const> const & vertices) {
			glCreateBuffers(1, (&this->bufferHandle));

			if (glGetError() == GL_NO_ERROR) {
				glNamedBufferStorage(
					this->bufferHandle,
					(sizeof(Vector4) * vertices.length),
					vertices.pointer,
					GL_DYNAMIC_STORAGE_BIT
				);

				if (glGetError() == GL_NO_ERROR) {
					glCreateVertexArrays(1, (&this->arrayHandle));

					if (glGetError() == GL_NO_ERROR) {
						glVertexArrayVertexBuffer(
							this->arrayHandle,
							0,
							this->bufferHandle,
							0,
							sizeof(Vector4)
						);

						if (glGetError() == GL_NO_ERROR) {
							glEnableVertexArrayAttrib(this->arrayHandle, 0);
							glVertexArrayAttribFormat(this->arrayHandle, 0, 4, GL_FLOAT, false, 0);
							glVertexArrayAttribBinding(this->arrayHandle, 0, 0);

							this->vertexCount = vertices.length;

							return true;
						}
					}
				}
			}

			return false;
		}
	};

	enum UniformBinding {
		ViewportBinding,
		RenderdataBinding,
	};

	struct GLShader {
		GLuint programHandle;

		GLuint renderdataBufferHandle;

		GLuint renderdataBufferLength;

		void DrawPolyInstanced(
			GLPolyBuffer const & polyBuffer,
			Material const & material,
			GLsizei count
		) {
			glBindTextureUnit(0, material.textureHandle);
			glBindBufferBase(GL_UNIFORM_BUFFER, RenderdataBinding, this->renderdataBufferHandle);
			glBindBuffer(GL_ARRAY_BUFFER, polyBuffer.bufferHandle);
			glBindVertexArray(polyBuffer.arrayHandle);
			glUseProgram(this->programHandle);
			glDrawArraysInstanced(GL_TRIANGLES, 0, polyBuffer.vertexCount, count);
		}

		void Free() {
			glDeleteBuffers(1, &this->renderdataBufferHandle);
			glDeleteProgram(this->programHandle);

			this->programHandle = 0;
			this->renderdataBufferHandle = 0;
			this->renderdataBufferLength = 0;
		}

		bool Load(Chars const & vertexSource, Chars const & fragmentSource, GLuint renderdataSize) {
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
						enum { ErrorBufferSize = 1024 };
						GLchar errorBuffer[ErrorBufferSize] = {};
						GLsizei errorBufferLength;

						glGetShaderInfoLog(
							shaderHandle,
							ErrorBufferSize,
							(&errorBufferLength),
							errorBuffer
						);

						Ona::Core::OutFile().Write($slice(errorBuffer).AsBytes());
					}
				}

				return 0;
			};

			GLuint const vertexObjectHandle = compileObject(vertexSource, GL_VERTEX_SHADER);

			// Did the vertex shader compile successfully?
			if (vertexObjectHandle) {
				GLuint const fragmentObjectHandle = compileObject(
					fragmentSource,
					GL_FRAGMENT_SHADER
				);

				// Did the fragment shader compile successfully?
				if (fragmentObjectHandle) {
					this->programHandle = glCreateProgram();

					// Did the shader program allocate?
					if (this->programHandle) {
						GLint success;

						glAttachShader(this->programHandle, vertexObjectHandle);
						glAttachShader(this->programHandle, fragmentObjectHandle);
						glLinkProgram(this->programHandle);
						glDetachShader(this->programHandle, vertexObjectHandle);
						glDetachShader(this->programHandle, fragmentObjectHandle);
						glDeleteShader(vertexObjectHandle);
						glDeleteShader(fragmentObjectHandle);
						glGetProgramiv(this->programHandle, GL_LINK_STATUS, (&success));

						// Did the shader program link properly?
						if (success) {
							glValidateProgram(this->programHandle);
							glGetProgramiv(this->programHandle, GL_LINK_STATUS, (&success));

							// Is the shader program valid?
							if (success) {
								glUniformBlockBinding(
									this->programHandle,
									glGetUniformBlockIndex(this->programHandle, "Viewport"),
									ViewportBinding
								);

								glUniformBlockBinding(
									this->programHandle,
									glGetUniformBlockIndex(this->programHandle, "Renderdata"),
									RenderdataBinding
								);

								glCreateBuffers(1, (&this->renderdataBufferHandle));

								if (glGetError() == GL_NO_ERROR) {
									glNamedBufferData(
										this->renderdataBufferHandle,
										renderdataSize,
										nullptr,
										GL_DYNAMIC_DRAW
									);

									if (glGetError() == GL_NO_ERROR) {
										this->renderdataBufferLength = renderdataSize;

										return true;
									}
								}

								return true;
							}
						}
					}
				}
			}

			return false;
		}

		GLuint WriteRenderdata(Slice<uint8_t const> const & renderdata) {
			uint8_t * mappedBuffer = reinterpret_cast<uint8_t *>(
				glMapNamedBuffer(this->renderdataBufferHandle, GL_WRITE_ONLY)
			);

			if (mappedBuffer) {
				// The number of bytes written will never be bigger than the max size of a GLBuffer.
				GLuint const bytesWritten = static_cast<GLuint>(CopyMemory(Slice<uint8_t>{
					.length = this->renderdataBufferLength,
					.pointer = mappedBuffer,
				}, renderdata));

				glUnmapNamedBuffer(this->renderdataBufferHandle);

				return bytesWritten;
			}

			return 0;
		}
	};

	GraphicsServer * LoadOpenGl(String const & title, int32_t width, int32_t height) {
		thread_local class OpenGlGraphicsServer final : public GraphicsServer {
			struct SpriteBatch {
				struct Chunk {
					enum { Max = 128 };

					Matrix transforms[Max];

					Vector4 viewports[Max];

					Vector4 tints[Max];
				};

				size_t count;

				Chunk chunk;
			};

			HashTable<Material *, PackedStack<SpriteBatch> *> spriteBatchSets;

			public:
			uint64_t timeNow, timeLast;

			SDL_Window * window;

			void * context;

			Point2 viewportSize;

			GLPolyBuffer quadPolyBuffer;

			GLShader canvasShader;

			GLuint viewportBufferHandle;

			bool isInitialized;

			OpenGlGraphicsServer(
				Allocator * allocator,
				String const & title,
				int32_t const width,
				int32_t const height
			) :
				spriteBatchSets{allocator},
				isInitialized{}
			{
				enum { InitFlags = SDL_INIT_VIDEO };

				if (SDL_Init(InitFlags) == 0) {
					enum {
						WindowPosition = SDL_WINDOWPOS_UNDEFINED,
						WindowFlags = (SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL)
					};

					// Fixes a bug on KDE desktops where launching the process disables the default
					// compositor.
					SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");

					this->window = SDL_CreateWindow(
						title.ZeroSentineled().Chars().pointer,
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

						// Initialize all immediately dependent resources.
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
										ViewportBinding,
										this->viewportBufferHandle
									);

									if (
										this->canvasShader.Load(
											canvasVertexSource,
											canvasFragmentSource,
											sizeof(SpriteBatch::Chunk)
										) &&
										this->quadPolyBuffer.Load($slice(quadVertices))
									) {
										glEnable(GL_DEBUG_OUTPUT);
										glEnable(GL_DEPTH_TEST);

										// Error logger
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

											outFile.Print(String{message});
											outFile.Print(String{"\n"});
										}, 0);

										glViewport(0, 0, width, height);

										this->viewportSize = Point2{width, height};
										this->isInitialized = true;
									}
								}
							}
						}
					}
				}
			}

			~OpenGlGraphicsServer() override {
				this->canvasShader.Free();
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
				Vector4 const rgba = color.Normalized();

				glClearColor(rgba.x, rgba.y, rgba.z, rgba.w);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			}

			Material * CreateMaterial(Image const & image) override {
				GLuint textureHandle;

				glCreateTextures(GL_TEXTURE_2D, 1, (&textureHandle));

				glTextureStorage2D(
					textureHandle,
					1,
					GL_RGBA8,
					image.dimensions.x,
					image.dimensions.y
				);

				// Was the texture allocated and initialized?
				if (glGetError() == GL_NO_ERROR) {
					glTextureSubImage2D(
						textureHandle,
						0,
						0,
						0,
						image.dimensions.x,
						image.dimensions.y,
						GL_RGBA,
						GL_UNSIGNED_BYTE,
						image.pixels
					);

					// Was the texture pixel data assigned?
					if (glGetError() == GL_NO_ERROR) {
						// Provided all prerequesites are met, these should not fail.
						glTextureParameteri(textureHandle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
						glTextureParameteri(textureHandle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
						glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
						glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

						return new Material{
							.dimensions = image.dimensions,
							.textureHandle = textureHandle,
						};
					}
				}

				return nullptr;
			}

			void DeleteMaterial(Material * & material) override {
				delete material;

				material = nullptr;
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
				if (this->spriteBatchSets.Count()) {
					Matrix * mappedViewport = reinterpret_cast<Matrix *>(
						glMapNamedBuffer(this->viewportBufferHandle, GL_WRITE_ONLY)
					);

					if (mappedViewport) {
						(*mappedViewport) = OrthographicMatrix(
							0,
							this->viewportSize.x,
							this->viewportSize.y,
							0,
							-1,
							1
						);

						glUnmapNamedBuffer(this->viewportBufferHandle);
					}

					this->spriteBatchSets.ForItems([this](
						Material * spriteMaterial,
						PackedStack<SpriteBatch> * spriteBatches
					) {
						if (spriteMaterial) {
							while (spriteBatches->Count()) {
								SpriteBatch * spriteBatch = (&spriteBatches->Peek());

								this->canvasShader.WriteRenderdata(AsBytes(spriteBatch->chunk));

								this->canvasShader.DrawPolyInstanced(
									this->quadPolyBuffer,
									*spriteMaterial,
									spriteBatch->count
								);

								spriteBatch->count = 0;

								spriteBatches->Pop();
							}
						}
					});

					this->spriteBatchSets.Clear();
				}

				SDL_GL_SwapWindow(this->window);
			}

			void RenderSprite(
				Material * spriteMaterial,
				Vector3 const & position,
				Color tint
			) override {
				if (spriteMaterial) {
					PackedStack<SpriteBatch> * * requiredBatches = this->spriteBatchSets.Require(
						spriteMaterial,

						[]() -> PackedStack<SpriteBatch> * {
							Allocator * allocator = DefaultAllocator();
							auto stack = new (allocator) PackedStack<SpriteBatch>{allocator};

							if (stack && (!stack->Push(SpriteBatch{}))) {
								delete (allocator, stack);

								return nullptr;
							}

							return stack;
						}
					);

					if (requiredBatches) {
						PackedStack<SpriteBatch> * batches = *requiredBatches;

						if (batches) {
							SpriteBatch * currentBatch = (&batches->Peek());

							if (currentBatch->count == SpriteBatch::Chunk::Max) {
								currentBatch = batches->Push(SpriteBatch{});

								if (!currentBatch) {
									// Out of memory.
									// TODO: Record failed render.
									return;
								}
							}

							currentBatch->chunk.transforms[currentBatch->count] = Matrix{
								static_cast<float>(spriteMaterial->dimensions.x), 0.f, 0.f, position.x,
								0.f, static_cast<float>(spriteMaterial->dimensions.y), 0.f, position.y,
								0.f, 0.f, 1.f, 0.f,
								0.f, 0.f, 0.f, 1.f
							};

							currentBatch->chunk.viewports[currentBatch->count] = Vector4{
								0.f,
								0.f,
								1.f,
								1.f
							};

							currentBatch->chunk.tints[currentBatch->count] = tint.Normalized();
							currentBatch->count += 1;
						}
					}

					// TODO: Record failed render.
				}
			}
		} graphicsServer = OpenGlGraphicsServer{DefaultAllocator(), title, width, height};

		return (graphicsServer.IsInitialized() ? &graphicsServer : nullptr);
	}
}
