
namespace Ona::Core {
	class Allocator : public Object {
		public:
		virtual Slice<uint8_t> Allocate(size_t size) = 0;

		virtual void Deallocate(void * allocation) = 0;

		virtual Slice<uint8_t> Reallocate(void * allocation, size_t size) = 0;
	};

	template<typename Type> struct Unique {
		Type value;

		private:
		bool exists;

		public:
		Unique() = default;

		Unique(Type const & value) : value{value}, exists{true} { };

		Unique(Unique const & that) = delete;

		~Unique() {
			if (this->exists) this->value.Free();
		}

		Type & Release() {
			this->exists = false;

			return this->value;
		}
	};

	size_t CopyMemory(Slice<uint8_t> destination, Slice<uint8_t const> source);

	Slice<uint8_t> WriteMemory(Slice<uint8_t> destination, uint8_t value);

	template<typename Type> Slice<uint8_t const> AsBytes(Type & value) {
		return SliceOf(&value, 1).AsBytes();
	}

	/**
	 * Zeroes the memory contents of `destination`.
	 */
	Slice<uint8_t> ZeroMemory(Slice<uint8_t> destination);
}
