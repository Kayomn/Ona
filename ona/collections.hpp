#ifndef COLLECTIONS_H
#define COLLECTIONS_H

#include "ona/core.hpp"

namespace Ona::Collections {
	template<typename Type> class Appender final {
		Ona::Core::Allocator * allocator;

		size_t count;

		Ona::Core::Slice<Type> values;

		public:
		Appender() = default;

		Appender(Ona::Core::Allocator * allocator) : allocator{allocator} { }

		Ona::Core::Allocator * AllocatorOf() {
			return this->allocator;
		}

		Ona::Core::Optional<Type *> Append(Type const & value) {
			using Opt = Ona::Core::Optional<Type *>;

			if (this->count >= this->values.length) {
				if (!this->Reserve(this->count ? this->count : 2)) {
					// Allocation failure.
					return Opt{};
				}
			}

			Type * bufferIndex = (this->values.pointer + this->count);
			this->count += 1;
			(*bufferIndex) = value;

			return Opt{bufferIndex};
		}

		Ona::Core::Slice<Type> AppendAll(Ona::Core::Slice<Type> const & values) {
			if (this->Reserve(values.length)) {
				Ona::Core::Slice<Type> range = this->values.Sliced(
					this->count,
					(this->count + values.length)
				);

				CopyMemory(range.AsBytes(), values.AsBytes());

				this->count += values.length;

				return range;
			}

			return Ona::Core::Slice<Type>{};
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

			return (this->values.pointer != nullptr);
		}

		void Free() {
			for (auto & value : this->Values()) value.~Type();

			if (this->allocator) {
				this->allocator->Deallocate(reinterpret_cast<uint8_t *>(this->values.pointer));
			} else {
				Ona::Core::Deallocate(reinterpret_cast<uint8_t *>(this->values.pointer));
			}
		}

		bool Reserve(size_t capacity) {
			if (this->allocator) {
				this->values = this->allocator->Reallocate(
					reinterpret_cast<uint8_t *>(this->values.pointer),
					(sizeof(Type) * (this->values.length + capacity)
				)).template As<Type>();
			} else {
				this->values = Ona::Core::Reallocate(
					reinterpret_cast<uint8_t *>(this->values.pointer),
					(sizeof(Type) * (this->values.length + capacity))
				).template As<Type>();
			}

			return (this->values.pointer != nullptr);
		}

		void Truncate(size_t n) {
			Ona::Core::Assert((n < this->count), Ona::Core::CharsFrom("Invalid range"));

			for (auto & value : this->values.Sliced((this->count - n), this->count)) value.~Type();

			this->count -= n;
		}

		Ona::Core::Slice<Type> Values() {
			return this->values.Sliced(0, this->values.length);
		}

		Ona::Core::Slice<Type const> Values() const {
			return this->values.Sliced(0, this->values.length);
		}
	};

	template<typename KeyType, typename ValueType> class Table final {
		public:
		struct Entry {
			KeyType key;

			ValueType value;
		};

		private:
		struct Bucket {
			Entry entry;

			Bucket * next;
		};

		static constexpr size_t defaultBufferSize = 256;

		using TableValue = Ona::Core::Optional<ValueType *>;

		Ona::Core::Optional<Ona::Core::Allocator *> allocator;

		size_t count;

		Ona::Core::Slice<Bucket *> buckets;

		public:
		Table() = default;

		Table(Ona::Core::Optional<Ona::Core::Allocator *> allocator) : allocator{allocator} { }

		size_t Count() const {
			return this->count;
		}

		template<typename CallableType> void ForEach(CallableType const & action) {
			size_t indexPointer = 0;

			if (indexPointer < this->buckets.length) {
				Bucket * bucket = this->buckets(indexPointer);

				while (bucket) {
					action(bucket->entry.key, bucket->entry.value);

					bucket = bucket->next;
				}

				indexPointer += 1;
			}
		}

		void Free() {

		}

		TableValue Insert(KeyType const & key, ValueType const & value) {
			return TableValue{};
		}

		bool Remove(KeyType const & key) {
			return false;
		}

		TableValue Lookup(KeyType const & key) {
			if (this->buckets.length) {
				Bucket * bucket = this->buckets(key.Hash() % this->buckets.length);

				if (bucket) while (bucket->entry.key != key) bucket = bucket->next;

				return (bucket ? TableValue{&bucket->entry.value} : TableValue{});
			}

			return TableValue{};
		}

		template<typename CallableType> TableValue LookupOrInsert(
			KeyType const & key,
			CallableType const & callable
		) {
			TableValue lookupValue = this->Lookup(key);

			if (lookupValue.HasValue()) return lookupValue.Value();

			this->Insert(key, callable());

			return this->Lookup(key);
		}
	};
}

#endif
