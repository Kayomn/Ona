
namespace Ona::Engine {
	struct Sprite {
		ResourceKey polyId;

		ResourceKey materialId;

		Vector2 dimensions;

		bool Equals(Sprite const & that) const;

		uint64_t ToHash() const;
	};

	class SpriteRenderer : public Object {
		struct Chunk {
			enum { Max = 128 };

			Matrix transforms[Max];

			Vector4 viewports[Max];
		};

		struct Batch {
			size_t count;

			Chunk chunk;
		};

		Allocator * allocator;

		ResourceKey rendererKey;

		ResourceKey rectPolyKey;

		HashTable<Sprite, PackedStack<Batch> *> batchSets;

		bool isInitialized;

		SpriteRenderer(GraphicsServer * graphicsServer);

		public:
		~SpriteRenderer() override;

		static SpriteRenderer * Acquire(GraphicsServer * graphicsServer);

		Result<Sprite> CreateSprite(GraphicsServer * graphicsServer, Image const & sourceImage);

		void DestroySprite(Sprite const & sprite);

		void Draw(Sprite const & sprite, Vector2 position);

		bool IsInitialized() const override;
	};
}
