#include "ona/engine/module.hpp"

namespace Ona::Engine {
	using namespace Ona::Core;
	using namespace Ona::Engine;

	struct Vertex2D {
		Vector2 position;

		Vector2 uv;
	};

	struct Material {
		Vector4 tintColor;
	};

	bool Sprite::Equals(Sprite const & that) const {
		return (this->ToHash() == that.ToHash());
	}

	void Sprite::Free() {

	}

	uint64_t Sprite::ToHash() const {
		return (
			(static_cast<uint64_t>(this->polyId) << 32) |
			static_cast<uint64_t>(this->materialId)
		);
	}

	SpriteRenderer::SpriteRenderer(GraphicsServer * graphicsServer) :
		allocator{DefaultAllocator()},
		batchSets{this->allocator},
		isInitialized{}
	{
		static Chars const vertexSource = CharsFrom(
			"#version 430 core\n"
			"#define INSTANCE_COUNT 128\n"
			"\n"
			"in vec2 quadVertex;\n"
			"in vec2 quadUv;\n"
			"\n"
			"out vec2 texCoords;\n"
			"out vec4 texTint;\n"
			"\n"
			"layout(std140, row_major) uniform Viewport {\n"
			"	mat4x4 projectionTransform;\n"
			"};\n"
			"\n"
			"layout(std140, row_major) uniform Renderer {\n"
			"	mat4x4 transforms[INSTANCE_COUNT];\n"
			"	vec4 viewports[INSTANCE_COUNT];\n"
			"};\n"
			"\n"
			"layout(std140, row_major) uniform Material {\n"
			"	vec4 tintColor;\n"
			"};\n"
			"\n"
			"uniform sampler2D spriteTexture;\n"
			"\n"
			"void main() {\n"
			"	const vec4 viewport = viewports[gl_InstanceID];\n"
			"\n"
			"	texCoords = ((quadUv * viewport.zw) + viewport.xy);\n"
			"	texTint = tintColor;\n"
			"\n"
			"	gl_Position = (\n"
			"		projectionTransform * transforms[gl_InstanceID] * vec4(quadVertex, 0.0, 1.0)"
			"	);\n"
			"}\n"
		);

		static Chars const fragmentSource = CharsFrom(
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

		static Property const vertexProperties[] = {
			Property{PropertyType::Float32, 2, String::From("quadVertex")},
			Property{PropertyType::Float32, 2, String::From("quadUv")}
		};

		static Property const rendererProperties[] = {
			Property{PropertyType::Float32, (16 * 128), String::From("transforms")},
			Property{PropertyType::Float32, (4 * 128), String::From("viewports")},
		};

		static Property const materialProperties[] = {
			Property{PropertyType::Float32, 4, String::From("tintColor")}
		};

		Result<ResourceKey> rendererKey = graphicsServer->CreateRenderer(
			vertexSource,
			fragmentSource,
			Slice<Property const>{.length = 2, .pointer = vertexProperties},
			Slice<Property const>{.length = 3, .pointer = rendererProperties},
			Slice<Property const>{.length = 1, .pointer = materialProperties}
		);

		if (rendererKey.IsOk()) {
			this->rendererKey = rendererKey.Value();

			static Vertex2D const quadPoly[] = {
				Vertex2D{Vector2{1.f, 1.f}, Vector2{1.f, 1.f}},
				Vertex2D{Vector2{1.f, 0.f}, Vector2{1.f, 0.f}},
				Vertex2D{Vector2{0.f, 1.f}, Vector2{0.f, 1.f}},
				Vertex2D{Vector2{1.f, 0.f}, Vector2{1.f, 0.f}},
				Vertex2D{Vector2{0.f, 0.f}, Vector2{0.f, 0.f}},
				Vertex2D{Vector2{0.f, 1.f}, Vector2{0.f, 1.f}}
			};

			Result<ResourceKey> polyKey = graphicsServer->CreatePoly(
				this->rendererKey,

				Slice<uint8_t const>{
					.length = (sizeof(Vertex2D) * 6),
					.pointer = reinterpret_cast<uint8_t const *>(quadPoly)
				}
			);

			if (polyKey.IsOk()) {
				this->rectPolyKey = polyKey.Value();
				this->isInitialized = true;

				graphicsServer->RegisterDispatcher([this, graphicsServer]() {
					if (this->batchSets.Count()) {
						Point2 const viewportSize = graphicsServer->ViewportOf().size;

						graphicsServer->UpdateProjection(
							OrthographicMatrix(0, viewportSize.x, viewportSize.y, 0, -1, 1)
						);

						this->batchSets.ForItems([
							this,
							graphicsServer
						](Sprite & sprite, PackedStack<Batch> * batches) {
							while (batches->Count()) {
								Batch * batch = (&batches->Peek());

								graphicsServer->UpdateRendererUserdata(
									this->rendererKey,
									AsBytes(batch->chunk)
								);

								graphicsServer->RenderPolyInstanced(
									this->rendererKey,
									sprite.polyId,
									sprite.materialId,
									batch->count
								);

								batch->count = 0;

								batches->Pop();
							}
						});

						this->batchSets.Clear();
					}
				});
			}
		}
	}

	SpriteRenderer::~SpriteRenderer() {
		this->batchSets.ForValues([this](PackedStack<Batch> * batches) {
			this->allocator->Destroy(batches);
		});

		// TODO: Graphics resources leak. Figure out how to solve this.
	}

	SpriteRenderer * SpriteRenderer::Acquire(GraphicsServer * graphicsServer) {
		thread_local SpriteRenderer instance = {graphicsServer};

		return &instance;
	}

	Result<Sprite> SpriteRenderer::CreateSprite(
		GraphicsServer * graphicsServer,
		Image const & sourceImage
	) {
		using Res = Result<Sprite>;

		Result<ResourceKey> materialKey = graphicsServer->CreateMaterial(
			this->rendererKey,
			sourceImage
		);

		if (materialKey.IsOk()) {
			Material material = {.tintColor = Vector4{1.f, 1.f, 1.f, 1.f}};

			graphicsServer->UpdateMaterialUserdata(materialKey.Value(), AsBytes(material));

			return Res::Ok(Sprite{
				.polyId = this->rectPolyKey,
				.materialId = materialKey.Value(),

				.dimensions = Vector2{
					static_cast<float>(sourceImage.dimensions.x),
					static_cast<float>(sourceImage.dimensions.y)
				}
			});
		}

		return Res::Fail();
	}

	void SpriteRenderer::Draw(Sprite const & sprite, Vector2 position) {
		PackedStack<Batch> * * requiredBatches = this->batchSets.Require(sprite, []() {
			Allocator * allocator = DefaultAllocator();
			PackedStack<Batch> * stack = allocator->New<PackedStack<Batch>>(allocator);

			if (stack && (!stack->Push(Batch{}))) allocator->Destroy(stack);

			return stack;
		});

		if (requiredBatches) {
			PackedStack<Batch> * batches = *requiredBatches;

			if (batches) {
				Batch * currentBatch = (&batches->Peek());

				if (currentBatch->count == Chunk::Max) {
					currentBatch = batches->Push(Batch{});

					if (!currentBatch) {
						// Out of memory.
						// TODO: Record failed render.
						return;
					}
				}

				currentBatch->chunk.transforms[currentBatch->count] = Matrix{
					sprite.dimensions.x, 0.f, 0.f, position.x,
					0.f, sprite.dimensions.y, 0.f, position.y,
					0.f, 0.f, 1.f, 0.f,
					0.f, 0.f, 0.f, 1.f
				};

				currentBatch->chunk.viewports[currentBatch->count] = Vector4{0.f, 0.f, 1.f, 1.f};
				currentBatch->count += 1;
			}
		}

		// TODO: Record failed render.
	}

	bool SpriteRenderer::IsInitialized() const {
		return (this->batchSets.IsInitialized() && this->isInitialized);
	}
}
