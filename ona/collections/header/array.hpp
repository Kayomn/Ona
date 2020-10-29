
namespace Ona::Collections {
	using namespace Ona::Core;

	template<typename ValueType, typename IndexType = size_t> class Array : Collection<ValueType> {
		virtual ValueType & At(IndexType index) = 0;

		virtual ValueType const & At(IndexType index) const = 0;

		virtual IndexType Count() const = 0;
	};

	template<
		typename ValueType,
		typename IndexType = size_t
	> class Appender : public Object, public Array<ValueType, IndexType> {
		private:
		ValueType * values;

		Allocator * allocator;

		IndexType capacity;

		IndexType count;

		public:
		Appender(Allocator * allocator) : allocator{allocator} { }

		~Appender() override {
			for (IndexType i = 0; i < this->count; i += 1) this->values[i].~ValueType();

			this->allocator->Deallocate(this->values);
		}

		Allocator * AllocatorOf() {
			return this->allocator;
		}

		Allocator const * AllocatorOf() const {
			return this->allocator;
		}

		ValueType * Append(ValueType const & value) {
			if (
				(this->count >= this->capacity) &&
				(!this->Reserve(this->count ? this->count : 2))
			) {
				// Allocation failure.
				return nullptr;
			}

			ValueType * bufferIndex = (this->values + this->count);
			this->count += 1;
			(*bufferIndex) = value;

			return bufferIndex;
		}

		Slice<ValueType> AppendAll(Slice<ValueType> const & values) {
			if (this->Reserve(values.length)) {
				Slice<ValueType> range = this->values.Sliced(
					this->count,
					(this->count + values.length)
				);

				CopyMemory(range.AsBytes(), values.AsBytes());

				this->count += values.length;

				return range;
			}

			return Slice<ValueType>{};
		}

		ValueType & At(IndexType index) override {
			return this->values[index];
		}

		ValueType const & At(IndexType index) const override {
			return this->values[index];
		}

		IndexType Capacity() const {
			return this->values.length;
		}

		void Clear() {
			for (IndexType i = 0; i < this->count; i += 1) this->values[i].~ValueType();

			this->count = 0;
		}

		IndexType Count() const override {
			return this->count;
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

		void ForValues(Callable<void(ValueType &)> const & action) override {
			for (IndexType i = 0; i < this->count; i += 1) action.Invoke(this->values[i]);
		}

		void ForValues(Callable<void(ValueType const &)> const & action) const override {
			for (IndexType i = 0; i < this->count; i += 1) action.Invoke(this->values[i]);
		}

		bool Reserve(IndexType capacity) {
			this->capacity += capacity;

			this->values = reinterpret_cast<ValueType *>(this->allocator->Reallocate(
				this->values,
				(sizeof(ValueType) * this->capacity)
			).pointer);

			bool const success = (this->values != nullptr);
			this->capacity *= success;
			this->count *= success;

			return success;
		}

		void Truncate(IndexType n) {
			assert((n < this->count) && "Invalid range");

			if constexpr (std::is_destructible_v<ValueType>) {
				for (IndexType i = (this->count - n); i < this->count; i += 1) {
					this->values[i].~ValueType();
				}
			}

			this->count -= n;
		}
	};
}
