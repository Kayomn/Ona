#ifndef COLLECTIONS_H
#define COLLECTIONS_H

#include "ona/core/header.hpp"

namespace Ona::Collections {
	using Ona::Core::Allocator;
	using Ona::Core::Optional;
	using Ona::Core::Slice;
	using Ona::Core::nil;

	template<typename Type> struct Appender {
		private:
		Slice<Type> values;

		Optional<Allocator *> allocator;

		size_t count;

		public:
		Optional<Allocator *> AllocatorOf() {
			return this->values.AllocatorOf();
		}

		Optional<Type *> Append(Type const & value) {
			if (this->count >= this->values.length) {
				if (!this->Reserve(this->count ? this->count : 2)) {
					// Allocation failure.
					return nil<Type *>;
				}
			}

			Type * bufferIndex = (this->values.pointer + this->count);
			this->count += 1;
			(*bufferIndex) = value;

			return bufferIndex;
		}

		Slice<Type> AppendAll(Slice<Type> const & values) {
			if (this->Reserve(values.length)) {
				Slice<Type> range = this->values.Sliced(
					this->count,
					(this->count + values.length)
				);

				CopyMemory(range.AsBytes(), values.AsBytes());

				this->count += values.length;

				return range;
			}

			return Slice<Type>{};
		}

		Type & At(size_t index) {
			return this->values(index);
		}

		Type const & At(size_t index) const {
			return this->values(index);
		}

		size_t Capacity() const {
			return this->values.length;
		}

		void Clear() {
			for (auto & value : this->Values()) value.~Type();

			this->count = 0;
		}

		size_t Count() const {
			return this->count;
		}

		bool Compress() {
			if (this->allocator) {
				this->values = this->allocator->Reallocate(
					this->values.pointer,
					(sizeof(Type) * this->count)
				).template As<Type>();
			} else {
				this->elements = Ona::Core::Reallocate(
					this->values.pointer,
					(sizeof(Type) * this->count)
				).template As<Type>();
			}

			return (this->values.HasValue());
		}

		void Free() {
			for (auto & value : this->Values()) value.~Type();

			if (this->allocator.HasValue()) {
				this->allocator->Deallocate(this->values.pointer);
			} else {
				Ona::Core::Deallocate(this->values.pointer);
			}
		}

		static void Init(Optional<Allocator *> allocator) {

		}

		bool Reserve(size_t capacity) {
			if (this->allocator.HasValue()) {
				this->values = this->allocator->Reallocate(
					this->values.pointer,
					(sizeof(Type) * (this->values.length + capacity)
				)).template As<Type>();
			} else {
				this->values = Ona::Core::Reallocate(
					this->values.pointer,
					(sizeof(Type) * (this->values.length + capacity))
				).template As<Type>();
			}

			return (this->values.HasValue());
		}

		void Truncate(size_t n) {
			assert((n < this->count) && "Invalid range");

			if constexpr (std::is_destructible_v<Type>) {
				for (auto & value : this->values.Sliced((this->count - n), this->count)) {
					value.~Type();
				}
			}

			this->count -= n;
		}

		Slice<Type> Values() {
			return this->values.Sliced(0, this->values.length);
		}

		Slice<Type const> Values() const {
			return this->values.Sliced(0, this->values.length);
		}
	};

	template<typename KeyType, typename ValueType> struct Table {
		public:
		struct Entry {
			KeyType key;

			ValueType value;
		};

		private:
		struct Bucket {
			Entry entry;

			Optional<Bucket *> next;
		};

		static constexpr size_t defaultHashSize = 256;

		Optional<Allocator *> allocator;

		size_t count;

		Slice<Optional<Bucket *>> buckets;

		Appender<Optional<Bucket *>> freedBuckets;

		Optional<Bucket *> CreateBucket(KeyType const & key, ValueType const & value) {
			if (this->freedBuckets.Count()) {
				return this->freedBuckets.At(this->freedBuckets.Count() - 1);
			} else {
				if (this->allocator.HasValue()) {
					Slice<uint8_t> allocation = this->allocator->Allocate(sizeof(Entry));

					if (allocation.HasValue()) {
						Bucket * bucket = reinterpret_cast<Bucket *>(allocation.pointer);
						(*bucket) = Bucket{Entry{key, value}, nil<Bucket *>};

						return bucket;
					}
				} else {
					Slice<uint8_t> allocation = Ona::Core::Allocate(sizeof(Entry));

					if (allocation.HasValue()) {
						Bucket * bucket = reinterpret_cast<Bucket *>(allocation.pointer);
						(*bucket) = Bucket{Entry{key, value}, nil<Bucket *>};

						return bucket;
					}
				}
			}

			return nil<Bucket *>;
		}

		public:
		Table() = default;

		Table(Optional<Allocator *> allocator) : allocator{allocator} { }

		Optional<Allocator *> AllocatorOf() {
			return this->allocator;
		}

		void Clear() {

		}

		size_t Count() const {
			return this->count;
		}

		template<typename CallableType> void ForEach(CallableType const & action) {
			size_t indexPointer = 0;

			while (indexPointer < this->buckets.length) {
				let bucket = this->buckets(indexPointer);

				while (bucket.HasValue()) {
					action(bucket->entry.key, bucket->entry.value);

					bucket = bucket->next;
				}

				indexPointer += 1;
			}
		}

		void Free() {

		}

		Optional<ValueType *> Insert(KeyType const & key, ValueType const & value) {
			if (!this->buckets.HasValue()) this->Rehash(defaultHashSize);

			uint64_t const hash = (key.Hash() % this->buckets.length);
			let bucket = this->buckets(hash);

			if (bucket.HasValue()) {
				while (bucket->next.HasValue()) bucket = bucket->next;

				bucket->next = this->CreateBucket(key, value);
			} else {
				let bucket = this->CreateBucket(key, value);
				this->buckets(hash) = bucket;

				if (bucket.HasValue()) return (&bucket->entry.value);
			}

			return nil<ValueType *>;
		}

		bool Remove(KeyType const & key) {
			return false;
		}

		bool Rehash(size_t tableSize) {
			let oldBuckets = this->buckets;

			if (this->allocator.HasValue()) {
				this->buckets = this->allocator->Allocate(
					tableSize * sizeof(Optional<Bucket *>)
				).template As<Optional<Bucket *>>();
			} else {
				this->buckets = Ona::Core::Allocate(
					tableSize * sizeof(Optional<Bucket *>)
				).template As<Optional<Bucket *>>();
			}

			if (this->buckets.HasValue()) {
				Ona::Core::ZeroMemory(this->buckets.AsBytes());

				if (oldBuckets.HasValue()) {
					size_t indexPointer = 0;

					if (indexPointer < oldBuckets.length) {
						let bucket = oldBuckets(indexPointer);

						while (bucket.HasValue()) {
							this->Insert(bucket->entry.key, bucket->entry.value);

							bucket = bucket->next;
						}

						indexPointer += 1;
					}
				}

				return true;
			}

			return false;
		}

		Optional<ValueType *> Lookup(KeyType const & key) {
			if (this->buckets.HasValue()) {
				let bucket = this->buckets(key.Hash() % this->buckets.length);

				if (bucket.HasValue()) {
					while (bucket->entry.key != key) bucket = bucket->next;

					if (bucket.HasValue()) return (&bucket->entry.value);
				}
			}

			return nil<ValueType *>;
		}

		template<typename CallableType> Optional<ValueType *> LookupOrInsert(
			KeyType const & key,
			CallableType const & callable
		) {
			Optional<ValueType *> lookupValue = this->Lookup(key);

			if (lookupValue.HasValue()) return lookupValue;

			this->Insert(key, callable());

			return this->Lookup(key);
		}
	};
}

#endif
