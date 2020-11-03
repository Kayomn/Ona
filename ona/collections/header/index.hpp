
namespace Ona::Collections {
	template<
		typename ValueType,
		typename IndexType = size_t
	> class Index : public Object, public Collection<ValueType> {
		private:
		struct Entry {
			union {
				ValueType occupied;

				IndexType vacant;
			};

			bool isOccupied;
		};

		Allocator * allocator;

		Entry * entries;

		IndexType capacity;

		IndexType count;

		IndexType nextFree;

		public:
		Index(Allocator * allocator) :
			allocator{allocator},
			entries{},
			capacity{},
			count{},
			nextFree{} { }

		~Index() override {
			this->allocator->Destroy(this->entries);
		}

		ValueType & At(IndexType const & index) {
			assert((index < this->count) && this->entries[index].isOccupied);

			return this->entries[index].occupied;
		}

		ValueType const & At(IndexType const & index) const {
			assert((index < this->count) && this->entries[index].isOccupied);

			return this->entries[index].occupied;
		}

		void Clear() override {
			this->count = 0;
		}

		void ForValues(Callable<void(ValueType &)> const & action) override {
			for (size_t i = 0; i < this->count; i += 1) {
				if (this->entries[i].isOccupied) action.Invoke(this->entries[i].occupied);
			}
		}

		void ForValues(Callable<void(ValueType const &)> const & action) const override {
			for (size_t i = 0; i < this->count; i += 1) {
				if (this->entries[i].isOccupied) action.Invoke(this->entries[i].occupied);
			}
		}

		IndexType Insert(ValueType const & value) {
			if (this->nextFree == this->capacity) {
				if (
					(this->count == this->capacity) &&
					(!this->Reserve(this->capacity ? this->capacity : 2))
				) {
					// Allocation failure.
					return 0;
				}

				IndexType const index = this->count;
				this->entries[index] = Entry{.occupied = value, .isOccupied = true};
				this->count += 1;

				return index;
			} else {
				IndexType const index = this->nextFree;
				this->nextFree = this->entries[index].vacant;
				this->entries[index] = Entry{.occupied = value, .isOccupied = true};
				this->count += 1;

				return index;
			}
		}

		size_t Count() const override {
			return this->count;
		}

		bool Remove(IndexType & key) {
			return false;
		}

		bool Reserve(IndexType capacity) {
			this->capacity += capacity;

			this->entries = reinterpret_cast<Entry *>(this->allocator->Reallocate(
				this->entries,
				(this->capacity * sizeof(Entry))
			).pointer);

			this->capacity *= (this->entries != nullptr);

			return (this->capacity != 0);
		}
	};
}
