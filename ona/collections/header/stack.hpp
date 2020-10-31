
namespace Ona::Collections {
	using namespace Ona::Core;

	template<typename ValueType> class Stack : public Collection<ValueType> {
		public:
		virtual ValueType & Peek() = 0;

		virtual ValueType const & Peek() const = 0;

		virtual void Pop() = 0;

		virtual ValueType * Push(ValueType const & value) = 0;
	};

	template<typename ValueType> class PackedStack : public Object, public Stack<ValueType> {
		private:
		Allocator * allocator;

		size_t count;

		Slice<ValueType> values;

		public:
		PackedStack(Allocator * allocator) : allocator{allocator}, values{}, count{} { }

		~PackedStack() {
			for (size_t i = 0; i < this->count; i += 1) this->values.At(i).~ValueType();
		}

		size_t Capacity() const {
			return this->values.length;
		}

		void Clear() {
			this->count = 0;
		}

		size_t Count() const {
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

		void Pop() override {
			this->count -= 1;
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
			this->values = this->allocator->Reallocate(
				this->values.pointer,
				((this->values.length + capacity) * sizeof(ValueType))
			).template As<ValueType>();

			return (this->values.length != 0);
		}
	};
}
