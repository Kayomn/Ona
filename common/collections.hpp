
namespace Ona {
	template<typename KeyType, typename ValueType> class HashTable final : public Object {
		private:
		struct Item {
			KeyType key;

			ValueType value;
		};

		struct Bucket {
			Item item;

			Bucket * next;
		};

		static constexpr uint32_t defaultHashSize = 256;

		Allocator * allocator;

		uint32_t count;

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
			count{},
			buckets{},
			freeBuckets{},
			loadMaximum{} { }

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

			uint32_t count = this->count;

			for (uint32_t i = 0; count != 0; i += 1) {
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

		void Clear() {
			if (this->count) {
				for (uint32_t i = 0; i < this->buckets.length; i += 1) {
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

		uint32_t Count() const {
			return this->count;
		}

		void ForEach(Callable<void(KeyType &, ValueType &)> const & action) {
			uint32_t count = this->count;

			for (uint32_t i = 0; count != 0; i += 1) {
				Bucket * bucket = this->buckets.At(i);

				while (bucket) {
					action.Invoke(bucket->item.key, bucket->item.value);

					count -= 1;
					bucket = bucket->next;
				}
			}
		}

		void ForEach(Callable<void(KeyType const &, ValueType const &)> const & action) const {
			uint32_t count = this->count;

			for (uint32_t i = 0; count != 0; i += 1) {
				Bucket * bucket = this->buckets.At(i);

				while (bucket) {
					action.Invoke(bucket->item.key, bucket->item.value);

					count -= 1;
					bucket = bucket->next;
				}
			}
		}

		void ForValues(Callable<void(ValueType &)> const & action) {
			uint32_t count = this->count;

			for (uint32_t i = 0; count != 0; i += 1) {
				Bucket * bucket = this->buckets.At(i);

				while (bucket) {
					action.Invoke(bucket->item.value);

					count -= 1;
					bucket = bucket->next;
				}
			}
		}

		void ForValues(Callable<void(ValueType const &)> const & action) const {
			uint32_t count = this->count;

			for (uint32_t i = 0; count != 0; i += 1) {
				Bucket * bucket = this->buckets.At(i);

				while (bucket) {
					action.Invoke(bucket->item.value);

					count -= 1;
					bucket = bucket->next;
				}
			}
		}

		ValueType * Insert(KeyType const & key, ValueType const & value) {
			if (!(this->buckets.length)) this->Rehash(defaultHashSize);

			uint32_t const hash = (KeyHash(key) % this->buckets.length);
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

		bool Rehash(uint32_t tableSize) {
			Slice<Bucket *> oldBuckets = this->buckets;

			this->buckets.pointer = reinterpret_cast<Bucket * *>(this->allocator->Allocate(
				tableSize * sizeof(Bucket * *)
			));

			if (this->buckets.pointer) {
				this->buckets.length = tableSize;

				ZeroMemory(this->buckets.Bytes());

				if (oldBuckets.length && this->count) {
					for (uint32_t i = 0; i < this->buckets.length; i += 1)  {
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

		ValueType * Lookup(KeyType const & key) {
			if (this->buckets.length && this->count) {
				Bucket * bucket = this->buckets.At(KeyHash(key) % this->buckets.length);

				if (bucket) {
					while (!KeyEquals(bucket->item.key, key)) bucket = bucket->next;

					if (bucket) return &bucket->item.value;
				}
			}

			return nullptr;
		}

		ValueType const * Lookup(KeyType const & key) const {
			if (this->buckets.length && this->count) {
				Bucket const * bucket = this->buckets.At(KeyHash(key) % this->buckets.length);

				if (bucket) {
					while (!KeyEquals(bucket->item.key, key)) bucket = bucket->next;

					if (bucket) return &bucket->item.value;
				}
			}

			return nullptr;
		}

		bool Remove(KeyType const & key) {
			// TODO: Implement.
			return false;
		}

		ValueType * Require(KeyType const & key, Callable<ValueType()> const & producer) {
			ValueType * lookupValue = this->Lookup(key);

			if (lookupValue) return lookupValue;

			return this->Insert(key, producer.Invoke());
		}
	};

	template<typename ValueType> class Sequence {
		public:
		virtual uint32_t Count() const = 0;

		virtual void ForEach(Callable<void(ValueType &)> const & action) = 0;

		virtual void ForEach(Callable<void(ValueType const &)> const & action) const = 0;
	};

	template<typename ValueType> class PackedStack final : public Object {
		private:
		Allocator * allocator;

		uint32_t count;

		Slice<ValueType> values;

		public:
		PackedStack(Allocator * allocator) : allocator{allocator}, values{}, count{} { }

		~PackedStack() override {
			for (uint32_t i = 0; i < this->count; i += 1) this->values.At(i).~ValueType();

			this->allocator->Deallocate(this->values.pointer);
		}

		Allocator * AllocatorOf() {
			return this->allocator;
		}

		Allocator const * AllocatorOf() const {
			return this->allocator;
		}

		ValueType & At(uint32_t index) {
			return this->values.At(index);
		}

		ValueType const & At(uint32_t index) const {
			return this->values.At(index);
		}

		uint32_t Capacity() const {
			return this->values.length;
		}

		void Clear() {
			for (uint32_t i = 0; i < this->count; i += 1) this->values.At(i).~ValueType();

			this->count = 0;
		}

		bool Compress() {
			this->values = reinterpret_cast<ValueType *>(this->allocator->Reallocate(
				this->values,
				(sizeof(ValueType) * this->count)
			).pointer);

			bool const success = (this->values != nullptr);
			this->capacity *= success;
			this->count *= success;

			return success;
		}

		uint32_t Count() const {
			return this->count;
		}

		void ForEach(Callable<void(ValueType &)> const & action) {
			for (uint32_t i = 0; i < this->count; i += 1) action.Invoke(this->values.At(i));
		}

		void ForEach(Callable<void(ValueType const &)> const & action) const {
			for (uint32_t i = 0; i < this->count; i += 1) action.Invoke(this->values.At(i));
		}

		ValueType & Peek() {
			return this->values.At(this->count - 1);
		}

		ValueType const & Peek() const {
			return this->values.At(this->count - 1);
		}

		void Pop(uint32_t n) {
			this->count -= n;
		}

		ValueType * Push(ValueType const & value) {
			if (
				(this->count == this->values.length) &&
				(!this->Reserve(this->values.length ? this->values.length : 2))
			) {
				// Allocation failure.
				return nullptr;
			}

			ValueType * valueIndex = (this->values.pointer + this->count);
			*valueIndex = value;
			this->count += 1;

			return valueIndex;
		}

		bool Reserve(uint32_t capacity) {
			uint32_t const newLength = (this->values.length + capacity);

			this->values.pointer = reinterpret_cast<ValueType *>(this->allocator->Reallocate(
				this->values.pointer,
				(newLength * sizeof(ValueType))
			));

			bool const success = (this->values.pointer != nullptr);

			this->values.length = (newLength * success);

			return success;
		}
	};

	template<typename ValueType> class PackedQueue final : public Object {
		Allocator * allocator;

		uint32_t count;

		Slice<ValueType> buffer;

		uint32_t tail;

		uint32_t head;

		public:
		PackedQueue(
			Allocator * allocator
		) : allocator{allocator}, count{}, buffer{}, tail{}, head{} {

		}

		~PackedQueue() override {
			this->allocator->Deallocate(this->buffer.pointer);
		}

		void Clear() {
			this->count = 0;
			this->head = 0;
			this->tail = 0;
		}

		uint32_t Count() const {
			return this->count;
		}

		ValueType & Dequeue() {
			uint32_t const oldHead = this->head;
			this->head = ((this->head + 1) % this->buffer.length);
			this->count -= 1;

			return this->buffer.At(oldHead);
		}

		ValueType * Enqueue(ValueType const & value) {
			if (
				(this->count == this->buffer.length) &&
				(!this->Reserve(this->buffer.length ? this->buffer.length : 2))
			) {
				// Allocation failure.
				return nullptr;
			}

			this->tail = ((this->tail + 1) % this->buffer.length);
			ValueType * bufferTail = (this->buffer.pointer + this->tail);
			(*bufferTail) = value;
			this->count += 1;

			return bufferTail;
		}

		void ForEach(Callable<void(ValueType &)> const & action) {

		}

		void ForEach(Callable<void(ValueType const &)> const & action) const {

		}

		bool Reserve(uint32_t capacity) {
			uint32_t const newLength = (this->buffer.length + capacity);

			this->buffer.pointer = reinterpret_cast<ValueType *>(this->allocator->Reallocate(
				this->buffer.pointer,
				(newLength * sizeof(ValueType))
			));

			bool const success = (this->buffer.pointer != nullptr);

			this->buffer.length = (newLength * success);

			return success;
		}
	};
}
