
namespace Ona::Collections {
	using namespace Ona::Core;

	template<typename ValueType> class Stack : public Collection<ValueType> {
		public:
		virtual ValueType & Peek() = 0;

		virtual ValueType const & Peek() const = 0;

		virtual void Pop(size_t n) = 0;

		virtual ValueType * Push(ValueType const & value) = 0;
	};

	template<typename ValueType> class PackedStack : public Stack<ValueType> {
		private:
		Allocator * allocator;

		size_t count;

		Slice<ValueType> values;

		public:
		PackedStack(Allocator * allocator) : allocator{allocator}, values{}, count{} { }

		~PackedStack() override {
			for (size_t i = 0; i < this->count; i += 1) this->values.At(i).~ValueType();

			this->allocator->Deallocate(this->values.pointer);
		}

		Allocator * AllocatorOf() {
			return this->allocator;
		}

		Allocator const * AllocatorOf() const {
			return this->allocator;
		}

		ValueType & At(size_t index) {
			return this->values.At(index);
		}

		ValueType const & At(size_t index) const {
			return this->values.At(index);
		}

		size_t Capacity() const {
			return this->values.length;
		}

		void Clear() override {
			for (size_t i = 0; i < this->count; i += 1) this->values.At(i).~ValueType();

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

		size_t Count() const override {
			return this->count;
		}

		virtual void ForValues(Callable<void(ValueType &)> const & action) override {
			for (size_t i = 0; i < this->count; i += 1) action.Invoke(this->values.At(i));
		}

		virtual void ForValues(Callable<void(ValueType const &)> const & action) const override {
			for (size_t i = 0; i < this->count; i += 1) action.Invoke(this->values.At(i));
		}

		ValueType & Peek() override {
			return this->values.At(this->count - 1);
		}

		ValueType const & Peek() const override {
			return this->values.At(this->count - 1);
		}

		void Pop(size_t n) override {
			this->count -= n;
		}

		ValueType * Push(ValueType const & value) override {
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

		bool Reserve(size_t capacity) {
			size_t const newLength = (this->values.length + capacity);

			this->values.pointer = reinterpret_cast<ValueType *>(this->allocator->Reallocate(
				this->values.pointer,
				(newLength * sizeof(ValueType))
			));

			bool const success = (this->values.pointer != nullptr);

			this->values.length = (newLength * success);

			return success;
		}
	};
}