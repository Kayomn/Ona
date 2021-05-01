
namespace Ona {
	/**
	 * A non-owning view, as defined by an address and length, into a particular region of memory.
	 *
	 * `Slice` can be considered a "fat pointer" of sorts.
	 */
	template<typename Type> struct Slice {
		size_t length;

		Type * pointer;

		template<typename CastType> using Casted = std::conditional_t<
			std::is_const<Type>::value,
			CastType const,
			CastType
		>;

		/**
		 * Reinterprets the `Slice` as a `Slice` of raw, unsigned bytes.
		 */
		constexpr Slice<Casted<uint8_t>> Bytes() const {
			return Slice<Casted<uint8_t>>{
				.length = (this->length * sizeof(Type)),
				.pointer = reinterpret_cast<Casted<uint8_t> *>(this->pointer)
			};
		}

		/**
		 * Provides mutable access to the value at index `index` of the `Slice.
		 */
		constexpr Type & At(size_t index) {
			assert((index < this->length));

			return this->pointer[index];
		}

		/**
		 * Provides non-mutable access to the value at index `index` of the `Slice.
		 */
		constexpr Type const & At(size_t index) const {
			assert((index < this->length));

			return this->pointer[index];
		}

		/**
		 * Checks for element-wise equality between the the `Slice` and `that`.
		 */
		constexpr bool Equals(Slice const & that) const {
			if (this->length != that.length) return false;

			for (size_t i = 0; i < this->length; i += 1) {
				if (this->pointer[i] != that.pointer[i]) return false;
			}

			return true;
		}

		/**
		 * Creates a new `Slice` of `const` `Type` from the `Slice`, granting access to elements
		 * from index `a` to position `b`.
		 */
		constexpr Slice<Type> Sliced(size_t a, size_t b) const {
			return Slice<Type>{
				.length = b,
				.pointer = (this->pointer + a)
			};
		}

		constexpr Type * begin() {
			return this->pointer;
		}

		constexpr Type const * begin() const {
			return this->pointer;
		}

		constexpr Type * end() {
			return (this->pointer + this->length);
		}

		constexpr Type const * end() const {
			return (this->pointer + this->length);
		}

		/**
		 * Provides casting to a version of the `Slice` with a `const` `Type`.
		 */
		constexpr operator Slice<Type const>() const {
			return (*reinterpret_cast<Slice<Type const> const *>(this));
		}
	};

	/**
	 * Creates a `Slice` from `pointer` that provides a view into the memory for `length` elements.
	 */
	template<typename Type> constexpr Slice<Type> SliceOf(Type * pointer, size_t const length) {
		return Slice<Type>{length, pointer};
	}

	using Chars = Slice<char const>;

	constexpr Chars CharsFrom(char const * pointer) {
		size_t length = 0;

		while (*(pointer + length)) length += 1;

		return SliceOf(pointer, length);
	}

	/**
	 * Checks if the error type is equal to `0` (no error).
	 */
	template<typename ErrorType> constexpr bool IsOk(ErrorType errorType) {
		static_assert(std::is_enum_v<ErrorType>, "Can only check enums for \"ok-ness\"");

		return (errorType == static_cast<ErrorType>(0));
	}

	/**
	 * Creates a `Slice` from `value` reinterpreted as bytes.
	 */
	template<typename Type> Slice<uint8_t const> AsBytes(Type const & value) {
		return Slice<uint8_t const>{
			.length = (1 * sizeof(Type)),
			.pointer = reinterpret_cast<uint8_t const *>(&value)
		};
	}

	template<typename> struct Callable;

	/**
	 * Type-erasing function-like wrapper for binding function pointers and functor structs alike.
	 */
	template<typename Return, typename... Args> struct Callable<Return(Args...)> {
		private:
		/**
		 * Dynamic dispatch table for `Callable`s.
		 *
		 * This table is necessary for the kind of type erasure that `Callable` uses, as things like
		 * copy-construction and proper destruction cannot happen without it.
		 */
		struct Operations {
			Return(* caller)(uint8_t const * userdata, Args... args);

			void(* copier)(uint8_t * destinationUserdata, uint8_t const * sourceUserdata);

			void(* destroyer)(uint8_t * userdata);
		};

		enum { BufferSize = 24 };

		Operations const * operations;

		uint8_t buffer[BufferSize];

		public:
		using Function = Return(*)(Args...);

		Callable() = default;

		Callable(Callable const & that) {
			if (that.HasValue()) {
				this->operations = that.operations;

				that.operations->copier(this->buffer, that.buffer);
			} else {
				this->operations = nullptr;

				for (size_t i = 0; i < BufferSize; i += 1) this->buffer[i] = 0;
			}
		}

		~Callable() {
			if (this->operations) this->operations->destroyer(this->buffer);
		}

		/**
		 * Constructs a `Callable` from the function pointer `function`.
		 */
		Callable(Function function) {
			static const Operations pointerOperations = {
				.caller = [](uint8_t const * userdata, Args... args) -> Return {
					return (*reinterpret_cast<Function const *>(userdata))(args...);
				},

				.copier = [](uint8_t * destinationUserdata, uint8_t const * sourceUserdata) {
					new (destinationUserdata) Function{
						*reinterpret_cast<Function const *>(sourceUserdata)
					};
				},

				.destroyer = [](uint8_t * userdata) {
					// Does nothing because function pointers are just numbers.
				},
			};

			this->operations = &pointerOperations;

			new (this->buffer) Function{function};
		}

		/**
		 * Constructs a `Callable` from the functor struct `functor`.
		 */
		template<typename Type> Callable(Type const & functor) {
			static const Operations functorOperations = {
				.caller = [](uint8_t const * userdata, Args... args) -> Return {
					return (*reinterpret_cast<Type const *>(userdata))(args...);
				},

				.copier = [](uint8_t * destinationUserdata, uint8_t const * sourceUserdata) {
					new (destinationUserdata) Type{
						*reinterpret_cast<Type const *>(sourceUserdata)
					};
				},

				.destroyer = [](uint8_t * userdata) {
					reinterpret_cast<Type *>(userdata)->~Type();
				},
			};

			static_assert((sizeof(Type) <= BufferSize), "Functor is too large");

			this->operations = &functorOperations;

			new (this->buffer) Type{functor};
		}

		/**
		 * Calls the `Callable` with `args` as the arguments.
		 *
		 * Calling `Callable::Invoke` on an empty `Callable` *will* result in a runtime error.
		 */
		Return Invoke(Args const &... args) const {
			assert(this->HasValue() && "Callable is empty");

			return this->operations->caller(this->buffer, args...);
		}

		/**
		 * Checks if the `Callable` contains a value.
		 */
		bool HasValue() const {
			return (this->operations != nullptr);
		}
	};

	class Object {
		public:
		Object() = default;

		Object(Object const &) = delete;

		virtual ~Object() {};

		Object & operator=(Object const &) = delete;
	};

	/**
	 * Reference-counted, UTF-8-encoded character sequence.
	 *
	 * `String` employs the use of small buffer optimization for text that is equal to or less than
	 * 24 bytes in size. Anything beyond this will involve the use of non-trivial resource
	 * allocation on the part of the system.
	 *
	 * `String` instances does not support text that is larger than 4GB in size.
	 */
	struct String {
		private:
		enum { StaticBufferSize = 24 };

		uint32_t size;

		uint32_t length;

		union {
			uint8_t * dynamic;

			uint8_t static_[StaticBufferSize];
		} buffer;

		public:
		String() = default;

		String(char const * data);

		String(Chars const & chars);

		String(char const c, uint32_t const count);

		String(String const & that);

		~String();

		/**
		 * Creates a `Slice` view of the `String` raw memory contents.
		 *
		 * The returned `Slice` is a weak reference to the `String` contents and may go out of scope
		 * at any time.
		 */
		Slice<uint8_t const> Bytes() const;

		/**
		 * Creates a `Chars` view of the `String` character contents.
		 *
		 * The returned `Chars` is a weak reference to the `String` contents and may go out of scope
		 * at any time.
		 */
		Chars Chars() const;

		/**
		 * Creates a new `String` from the concatenated contents of `args`.
		 */
		static String Concat(std::initializer_list<String> const & args);

		/**
		 * Checks if the `String` ends with the characters in `string`.
		 */
		bool EndsWith(String const & string) const;

		/**
		 * Checks if the `String` is lexicographically equivalent to `that`.
		 */
		bool Equals(String const & that) const;

		/**
		 * Checks if the `String` uses non-trivial storage for its contents.
		 */
		constexpr bool IsDynamic() const {
			return (this->size > StaticBufferSize);
		}

		/**
		 * Retrieves the length of the `String`.
		 *
		 * As `String` instances are immutable, this value is guaranteed to never change after
		 * instantiation.
		 */
		constexpr uint32_t Length() const {
			return this->length;
		}

		/**
		 * Computes the hash value of the `String` contents.
		 */
		uint64_t ToHash() const;

		/**
		 * Creates a copy of the `String` instance.
		 */
		String ToString() const;

		/**
		 * Creates a copy of the `String` instance with a zero sentinel character on the end of the
		 * data, allowing it to be used with C APIs that expect character sequences to end with a
		 * sentinel.
		 */
		String ZeroSentineled() const;
	};

	template<typename Type> class Array : public Object {
		public:
		/**
		 * Retrieves the value by reference at `index`.
		 *
		 * Specifying an invalid `index` will result in a runtime error.
		 */
		virtual Type & At(size_t index) = 0;

		/**
		 * Retrieves the value by reference at `index`.
		 *
		 * Specifying an invalid `index` will result in a runtime error.
		 */
		virtual Type const & At(size_t index) const = 0;

		/**
		 * Gets the number of elements in the `Array`.
		 */
		virtual size_t Length() const = 0;

		/**
		 * Gets a pointer to the head of the `Array`.
		 */
		virtual Type * Pointer() = 0;

		/**
		 * Gets a pointer to the head of the `Array`.
		 */
		virtual Type const * Pointer() const = 0;

		/**
		 * Creates a `Slice` of the `Array` from element `a` of `b` elements long.
		 *
		 * Once the `Array` exits scope then the `Slice` should be considered invalid.
		 *
		 * Specifying an invalid range with `a` and `b` will result in a runtime error.
		 */
		virtual Slice<Type> Sliced(size_t a, size_t b) = 0;

		/**
		 * Creates a `Slice` of the `Array` from element `a` of `b` elements long.
		 *
		 * Once the `Array` exits scope then the `Slice` should be considered invalid.
		 *
		 * Specifying an invalid range with `a` and `b` will result in a runtime error.
		 */
		virtual Slice<Type const> Sliced(size_t a, size_t b) const = 0;

		/**
		 * Creates a `Slice` of all elements in the `Array`.
		 *
		 * Once the `Array` exits scope then the `Slice` should be considered invalid.
		 *
		 * Specifying an invalid range with `a` and `b` will result in a runtime error.
		 */
		virtual Slice<Type> Values() = 0;

		/**
		 * Creates a `Slice` of all elements in the `Array`.
		 *
		 * Once the `Array` exits scope then the `Slice` should be considered invalid.
		 *
		 * Specifying an invalid range with `a` and `b` will result in a runtime error.
		 */
		virtual Slice<Type const> Values() const = 0;
	};

	/**
	 * Common interface for interfacing with "stream-like" APIs like the filesystem and file
	 * servers.
	 */
	class Stream : public Object {
		public:
		/**
		 * Bitflags used for indicating the access state of a `Stream`.
		 */
		enum OpenFlags {
			OpenUnknown = 0,
			OpenRead = 0x1,
			OpenWrite = 0x2
		};

		virtual uint64_t AvailableBytes() = 0;

		virtual String ID() = 0;

		virtual uint64_t ReadBytes(Slice<uint8_t> input) = 0;

		virtual uint64_t ReadUtf8(Slice<char> input) = 0;

		virtual int64_t SeekHead(int64_t offset) = 0;

		virtual int64_t SeekTail(int64_t offset) = 0;

		virtual int64_t Skip(int64_t offset) = 0;

		virtual uint64_t WriteBytes(Slice<uint8_t const> const & output) = 0;
	};

	struct AtomicU32 {
		private:
		_Atomic uint32_t value;

		public:
		AtomicU32() = default;

		AtomicU32(AtomicU32 const & that);

		AtomicU32(uint32_t value);

		uint32_t FetchAdd(uint32_t amount);

		uint32_t FetchSub(uint32_t amount);

		uint32_t Load() const;

		void Store(uint32_t value);
	};

	/**
	 * Identifier for memory allocation strategies.
	 */
	enum class Allocator {
		Default,
	};
}
