
namespace Ona::Collections {
	template<typename ValueType> class Queue : public Collection<ValueType> {
		public:
		virtual ValueType & Dequeue() = 0;

		virtual ValueType * Enqueue(ValueType const & value) = 0;
	};

	template<typename ValueType> class PackedQueue final : public Queue<ValueType> {
		Allocator * allocator;

		size_t count;

		Slice<ValueType> buffer;

		size_t tail;

		size_t head;

		public:
		PackedQueue(
			Allocator * allocator
		) : allocator{allocator}, count{}, buffer{}, tail{}, head{} {

		}

		~PackedQueue() override {
			this->allocator->Deallocate(this->buffer.pointer);
		}

		void Clear() override {
			this->count = 0;
			this->head = 0;
			this->tail = 0;
		}

		size_t Count() const override {
			return this->count;
		}

		ValueType & Dequeue() override {
			size_t const oldHead = this->head;
			this->head = ((this->head + 1) % this->buffer.length);
			this->count -= 1;

			return this->buffer.At(oldHead);
		}

		ValueType * Enqueue(ValueType const & value) override {
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

		void ForValues(Callable<void(ValueType &)> const & action) override {

		}

		void ForValues(Callable<void(ValueType const &)> const & action) const override {

		}

		bool Reserve(size_t capacity) {
			size_t const newLength = (this->buffer.length + capacity);

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
