#ifndef COLLECTIONS_H
#define COLLECTIONS_H

#include "ona/core.hpp"

namespace Ona::Collections {
	using Ona::Core::Allocator;
	using Ona::Core::Reallocate;
	using Ona::Core::Slice;

	template<typename Type> class Appender final {
		Allocator * allocator;

		size_t count;

		Slice<Type> values;

		public:
		Appender() = default;

		Appender(Allocator * allocator) : allocator{allocator} { }

		~Appender() {
			for (auto & value : this->Values()) value.~Type();

			if (this->allocator) {
				this->allocator->Deallocate(reinterpret_cast<uint8_t *>(this->values.pointer));
			} else {
				Ona::Core::Deallocate(reinterpret_cast<uint8_t *>(this->values.pointer));
			}
		}

		Allocator * AllocatorOf() {
			return this->allocator;
		}

		Type * Append(Type const & value) {
			if (this->count >= this->values.length) {
				if (!this->Reserve(this->count ? this->count : 2)) {
					// Allocation failure.
					return nullptr;
				}
			}

			Type * bufferIndex = (this->values.pointer + this->count);
			this->count += 1;
			(*bufferIndex) = value;

			return bufferIndex;
		}

		Slice<Type> AppendAll(Slice<Type> const & values) {
			if (this->Reserve(values.length)) {
				Slice<Type> range = this->values.Sliced(this->count, (this->count + values.length));

				for (size_t i = 0; i < range.length; i += 1) range[i] = values[i];

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
				this->elements = Reallocate(
					this->values.pointer,
					(sizeof(Type) * this->count)
				).template As<Type>();
			}

			return (this->values.pointer != nullptr);
		}

		bool Reserve(size_t capacity) {
			if (this->allocator) {
				this->values = this->allocator->Reallocate(
					reinterpret_cast<uint8_t *>(this->values.pointer),
					(sizeof(Type) * (this->values.length + capacity)
				)).template As<Type>();
			} else {
				this->values = Reallocate(
					reinterpret_cast<uint8_t *>(this->values.pointer),
					(sizeof(Type) * (this->values.length + capacity)
				)).template As<Type>();
			}

			return (this->values.pointer != nullptr);
		}

		void Truncate(size_t n) {
			Assert((n < this->count), "Invalid range");

			for (auto & value : this->values.Sliced((this->count - n), this->count)) {
				value.~Type();
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
}

#endif
