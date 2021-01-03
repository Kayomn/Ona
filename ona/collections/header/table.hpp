
namespace Ona::Collections {
	using namespace Ona::Core;

	template<
		typename KeyType,
		typename ValueType
	> class Table : public Collection<ValueType> {
		public:
		virtual void ForItems(Callable<void(KeyType &, ValueType &)> const & action) = 0;

		virtual void ForItems(
			Callable<void(KeyType const &, ValueType const &)> const & action
		) const = 0;

		virtual ValueType * Insert(KeyType const & key, ValueType const & value) = 0;

		virtual ValueType * Lookup(KeyType const & key) = 0;

		virtual ValueType const * Lookup(KeyType const & key) const = 0;

		virtual bool Remove(KeyType const & key) = 0;

		virtual ValueType * Require(
			KeyType const & key,
			Callable<ValueType()> const & producer
		) = 0;
	};

	template<
		typename KeyType,
		typename ValueType
	> class HashTable : public Object, public Table<KeyType, ValueType> {
		private:
		struct Item {
			KeyType key;

			ValueType value;
		};

		struct Bucket {
			Item item;

			Bucket * next;
		};

		static constexpr size_t defaultHashSize = 256;

		Allocator * allocator;

		size_t count;

		Slice<Bucket *> buckets;

		Bucket * freeBuckets;

		float loadMaximum;

		Bucket * CreateBucket(KeyType const & key, ValueType const & value) {
			if (this->freeBuckets) {
				Bucket * bucket = this->freeBuckets;
				this->freeBuckets = this->freeBuckets->next;
				(*bucket) = Bucket{Item{key, value}, nullptr};

				return bucket;
			} else {
				Bucket * bucket = reinterpret_cast<Bucket *>(
					this->allocator->Allocate(sizeof(Bucket))
				);

				if (bucket) {
					(*bucket) = Bucket{Item{key, value}, nullptr};

					return bucket;
				}
			}

			return nullptr;
		}

		public:
		HashTable(Allocator * allocator) :
			allocator{allocator},
			count{0},
			buckets{0},
			freeBuckets{0},
			loadMaximum{0} { }

		~HashTable() override {
			auto destroyChain = [this](Bucket * bucket) {
				while (bucket) {
					Bucket * nextBucket = bucket->next;

					bucket->item.key.~KeyType();
					bucket->item.value.~ValueType();
					this->allocator->Deallocate(bucket);

					bucket = nextBucket;
				}
			};

			destroyChain(this->freeBuckets);

			size_t count = this->count;

			for (size_t i = 0; count != 0; i += 1) {
				Bucket * bucket = this->buckets.At(i);

				if (bucket) {
					destroyChain(bucket);

					count -= 1;
				}
			}

			this->allocator->Deallocate(this->buckets.pointer);
		}

		Allocator * AllocatorOf() {
			return this->allocator;
		}

		Allocator const * AllocatorOf() const {
			return this->allocator;
		}

		void Clear() override {
			if (this->count) {
				for (size_t i = 0; i < this->buckets.length; i += 1) {
					Bucket * rootBucket = this->buckets.At(i);

					if (rootBucket) {
						rootBucket->item.key.~KeyType();
						rootBucket->item.value.~ValueType();

						Bucket * bucket = rootBucket;

						while (bucket->next) {
							bucket->next->item.key.~KeyType();
							bucket->next->item.value.~ValueType();

							bucket = bucket->next;
						}

						bucket->next = this->freeBuckets;
						this->freeBuckets = rootBucket;
						this->buckets.At(i) = nullptr;
					}
				}

				this->count = 0;
			}
		}

		size_t Count() const override {
			return this->count;
		}

		void ForItems(Callable<void(KeyType &, ValueType &)> const & action) override {
			size_t count = this->count;

			for (size_t i = 0; count != 0; i += 1) {
				Bucket * bucket = this->buckets.At(i);

				while (bucket) {
					action.Invoke(bucket->item.key, bucket->item.value);

					count -= 1;
					bucket = bucket->next;
				}
			}
		}

		void ForItems(
			Callable<void(KeyType const &, ValueType const &)> const & action
		) const override {
			size_t count = this->count;

			for (size_t i = 0; count != 0; i += 1) {
				Bucket * bucket = this->buckets.At(i);

				while (bucket) {
					action.Invoke(bucket->item.key, bucket->item.value);

					count -= 1;
					bucket = bucket->next;
				}
			}
		}

		void ForValues(Callable<void(ValueType &)> const & action) override {
			size_t count = this->count;

			for (size_t i = 0; count != 0; i += 1) {
				Bucket * bucket = this->buckets.At(i);

				while (bucket) {
					action.Invoke(bucket->item.value);

					count -= 1;
					bucket = bucket->next;
				}
			}
		}

		void ForValues(Callable<void(ValueType const &)> const & action) const override {
			size_t count = this->count;

			for (size_t i = 0; count != 0; i += 1) {
				Bucket * bucket = this->buckets.At(i);

				while (bucket) {
					action.Invoke(bucket->item.value);

					count -= 1;
					bucket = bucket->next;
				}
			}
		}

		ValueType * Insert(KeyType const & key, ValueType const & value) override {
			if (!(this->buckets.length)) this->Rehash(defaultHashSize);

			size_t const hash = (KeyHash(key) % this->buckets.length);
			Bucket * bucket = this->buckets.At(hash);

			if (bucket) {
				if (KeyEquals(bucket->item.key, key)) {
					bucket->item.value = value;

					return (&bucket->item.value);
				} else {
					while (bucket->next) {
						bucket = bucket->next;

						if (KeyEquals(bucket->item.key, key)) {
							bucket->item.value = value;

							return &bucket->item.value;
						}
					}

					bucket->next = this->CreateBucket(key, value);

					return (bucket->next ? &bucket->next->item.value : nullptr);
				}

				bucket = bucket->next;
			} else {
				bucket = this->CreateBucket(key, value);
				this->buckets.At(hash) = bucket;
				this->count += 1;

				return &bucket->item.value;
			}

			if (this->loadMaximum && this->LoadFactor() > this->loadMaximum) {
				this->Rehash(this->buckets.length * 2);
			}

			return nullptr;
		}

		bool Rehash(size_t tableSize) {
			Slice<Bucket *> oldBuckets = this->buckets;

			this->buckets.pointer = reinterpret_cast<Bucket * *>(this->allocator->Allocate(
				tableSize * sizeof(size_t)
			));

			if (this->buckets.pointer) {
				this->buckets.length = tableSize;

				ZeroMemory(this->buckets.AsBytes());

				if (oldBuckets.length && this->count) {
					for (size_t i = 0; i < this->buckets.length; i += 1)  {
						Bucket * bucket = oldBuckets.At(i);

						while (bucket) {
							this->Insert(bucket->item.key, bucket->item.value);

							bucket = bucket->next;
						}
					}

					this->allocator->Deallocate(oldBuckets.pointer);
				}

				return true;
			}

			return false;
		}

		static bool KeyEquals(KeyType const & a, KeyType const & b) {
			if constexpr (std::is_pointer_v<KeyType>) {
				return (a == b);
			} else {
				return a.Equals(b);
			}
		}

		static uint64_t KeyHash(KeyType const & key) {
			if constexpr (std::is_pointer_v<KeyType>) {
				if constexpr (std::is_polymorphic_v<std::remove_pointer<KeyType>>) {
					return key->ToHash();
				} else {
					return reinterpret_cast<uint64_t>(key);
				}
			} else {
				return key.ToHash();
			}
		}

		float LoadFactor() const {
			return (this->count / this->buckets.length);
		}

		ValueType * Lookup(KeyType const & key) override {
			if (this->buckets.length && this->count) {
				Bucket * bucket = this->buckets.At(KeyHash(key) % this->buckets.length);

				if (bucket) {
					while (!KeyEquals(bucket->item.key, key)) bucket = bucket->next;

					if (bucket) return &bucket->item.value;
				}
			}

			return nullptr;
		}

		ValueType const * Lookup(KeyType const & key) const override {
			if (this->buckets.length && this->count) {
				Bucket const * bucket = this->buckets.At(KeyHash(key) % this->buckets.length);

				if (bucket) {
					while (!KeyEquals(bucket->item.key, key)) bucket = bucket->next;

					if (bucket) return &bucket->item.value;
				}
			}

			return nullptr;
		}

		bool Remove(KeyType const & key) override {
			// TODO: Implement.
			return false;
		}

		ValueType * Require(KeyType const & key, Callable<ValueType()> const & producer) override {
			ValueType * lookupValue = this->Lookup(key);

			if (lookupValue) return lookupValue;

			return this->Insert(key, producer.Invoke());
		}
	};
}
