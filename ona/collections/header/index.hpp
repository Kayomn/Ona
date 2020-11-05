
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

			static constexpr Entry Ouccipied(ValueType const & value) {
				return Entry{
					.occupied = value,
					.isOccupied = true
				};
			}

			static constexpr Entry Vacant(IndexType const value) {
				return Entry{
					.vacant = value,
					.isOccupied = false
				};
			}
		};

		Allocator * allocator;

		Entry * entries;

		IndexType capacity;

		IndexType count;

		IndexType nextFree;

		bool IsValidKey(IndexType const key) const {
			return ((key < this->count) && this->entries[key].isOccupied);
		}

		public:
		Index(Allocator * allocator) :
			allocator{allocator},
			entries{},
			capacity{},
			count{},
			nextFree{} { }

		~Index() override {
			for (IndexType i = 0; i < this->count; i += 1) {
				if (this->entries[i].isOccupied) this->entries[i].occupied.~ValueType();
			}

			this->allocator->Destroy(this->entries);
		}

		void Clear() override {
			for (IndexType i = 0; i < this->count; i += 1) {
				if (this->entries[i].isOccupied) this->entries[i].occupied.~ValueType();
			}

			this->count = 0;
			this->nextFree = 0;
		}

		size_t Count() const override {
			return this->count;
		}

		void ForValues(Callable<void(ValueType &)> const & action) override {
			for (IndexType i = 0; i < this->count; i += 1) {
				if (this->entries[i].isOccupied) action.Invoke(this->entries[i].occupied);
			}
		}

		void ForValues(Callable<void(ValueType const &)> const & action) const override {
			for (IndexType i = 0; i < this->count; i += 1) {
				if (this->entries[i].isOccupied) action.Invoke(this->entries[i].occupied);
			}
		}

		Result<IndexType> Insert(ValueType const & value) {
			using Res = Result<IndexType>;
			IndexType index;

			if (this->nextFree == this->capacity) {
				if (
					(this->count == this->capacity) &&
					(!this->Reserve(this->capacity ? this->capacity : 2))
				) {
					// Allocation failure.
					return Res::Fail();
				}

				index = this->count;
				this->nextFree += 1;
			} else {
				index = this->nextFree;
				this->nextFree = this->entries[index].vacant;
			}

			this->entries[index] = Entry::Ouccipied(value);
			this->count += 1;

			return Res::Ok(index);
		}

		ValueType * Lookup(IndexType const key) {
			return (this->IsValidKey(key) ? (&this->entries[key].occupied) : nullptr);
		}

		ValueType const * Lookup(IndexType const key) const {
			return (this->IsValidKey(key) ? (&this->entries[key].occupied) : nullptr);
		}

		bool Remove(IndexType const key) {
			if (this->IsValidKey(key)) {
				this->entries[key] = Entry::Vacant(this->nextFree);
				this->nextFree = key;

				return true;
			}

			return false;
		}

		bool Reserve(IndexType const capacity) {
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
