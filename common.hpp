#ifndef ONA_COMMON_H
#define ONA_COMMON_H

#include <cassert>
#include <stddef.h>
#include <stdint.h>
#include <new>
#include <initializer_list>

#define $packed __attribute__ ((packed))

#define $length(arr) (sizeof(arr) / sizeof(*arr))

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

	constexpr bool IsAlpha(int const c) {
		return (((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z')));
	}

	constexpr bool IsDigit(int const c) {
		return ((c >= '0') && (c <= '9'));
	}

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

	/**
	 * Creates a `String` containing `value` as signed decimal-encoded text.
	 */
	String DecStringSigned(int64_t value);

	/**
	 * Creates a `String` containing `value` as unsigned decimal-encoded text.
	 */
	String DecStringUnsigned(uint64_t value);

	/**
	 * Creates a new `String` from `string` with the format arguments in `values`.
	 *
	 * Failure to supply a value for a format placeholder in the `string` will result in that
	 * placeholder being left in the produced `String`.
	 *
	 * Excess format specifiers are ignored.
	 */
	String Format(String const & string, std::initializer_list<String> const & values);

	/**
	 * Attempts to parse `string` as if it were a signed decimal integer, writing the result into
	 * `output` if `string` could be parsed and returning `true`, otherwise `false` if the operation
	 * failed.
	 */
	bool ParseSigned(String const & string, int64_t & output);

	/**
	 * Attempts to parse `string` as if it were an unsigned decimal integer, writing the result into
	 * `output` if `string` could be parsed and returning `true`, otherwise `false` if the operation
	 * failed.
	 */
	bool ParseUnsigned(String const & string, uint64_t & output);

	/**
	 * Attempts to parse `string` as if it were a floating point number, writing the result into
	 * `output` if `string` could be parsed and returning `true`, otherwise `false` if the operation
	 * failed.
	 */
	bool ParseFloating(String const & string, double & output);

	/**
	 * File operations table used for defining the behavior of the file operations.
	 */
	struct FileOperations {
		enum class SeekMode {
			Cursor,
			Head,
			Tail,
		};

		Callable<void(void * handle)> closer;

		Callable<size_t(void * handle, Slice<uint8_t> output)> reader;

		Callable<int64_t(void * handle, int64_t offset, SeekMode mode)> seeker;

		Callable<size_t(void * handle, Slice<uint8_t const> const & input)> writer;
	};

	/**
	 * Interface to a file-like data resource.
	 */
	struct FileAccess {
		private:
		void * handle;

		FileOperations const * operations;

		public:
		/**
		 * Bitflags used for indicating the access state of a `File`.
		 */
		enum OpenFlags {
			OpenUnknown = 0,
			OpenRead = 0x1,
			OpenWrite = 0x2
		};

		FileAccess() = default;

		FileAccess(
			FileOperations const * operations,
			void * handle
		) : handle{handle}, operations{operations} {

		}

		/**
		 * Frees the resources associated with the `File`.
		 */
		void Free() {
			if (this->operations && this->operations->closer.HasValue()) {
				this->operations->closer.Invoke(this->handle);

				this->handle = nullptr;
			}
		}

		/**
		 * Attempts to read the contents of the file for as many bytes as `input` is in size into
		 * `input`.
		 *
		 * The number of bytes in `input` successfully written from the file are returned.
		 */
		size_t Read(Slice<uint8_t> input) {
			if (this->operations && this->operations->reader.HasValue()) {
				return this->operations->reader.Invoke(this->handle, input);
			}

			return 0;
		}

		/**
		 * Seeks `offset` bytes into the file from the ending of the file.
		 *
		 * The number of successfully sought bytes are returned.
		 */
		int64_t SeekHead(int64_t offset) {
			if (this->operations && this->operations->seeker.HasValue()) {
				return this->operations->seeker.Invoke(
					this->handle,
					offset,
					FileOperations::SeekMode::Head
				);
			}

			return 0;
		}

		/**
		 * Seeks `offset` bytes into the file from the beginning of the file.
		 *
		 * The number of successfully sought bytes are returned.
		 */
		int64_t SeekTail(int64_t offset) {
			if (this->operations && this->operations->seeker.HasValue()) {
				return this->operations->seeker.Invoke(
					this->handle,
					offset,
					FileOperations::SeekMode::Tail
				);
			}

			return 0;
		}

		/**
		 * Skips `offset` bytes in the file from the current cursor position.
		 *
		 * The number of successfully skipped bytes are returned.
		 */
		int64_t Skip(int64_t offset) {
			if (this->operations && this->operations->seeker.HasValue()) {
				return this->operations->seeker.Invoke(
					this->handle,
					offset,
					FileOperations::SeekMode::Cursor
				);
			}

			return 0;
		}

		/**
		 * Attempts to write the contents `output` to the file directly without buffered I/O.
		 *
		 * The number of bytes in `output` successfully written to the file are returned.
		 */
		size_t Write(Slice<uint8_t const> const & output) {
			if (this->operations && this->operations->writer.HasValue()) {
				return this->operations->writer.Invoke(this->handle, output);
			}

			return 0;
		}
	};

	class Allocator;

	struct FileContents {
		Allocator * allocator;

		Slice<uint8_t> raw;

		void Free();

		String ToString() const;
	};

	enum class FileLoadError {
		None,
		FileError,
		OutOfMemory,
	};

	/**
	 * Attempts to open the file specified at `filePath` using `openFlags`.
	 *
	 * A successfully opened file will be written to `result` and `true` is returned. Otherwise,
	 * `false` is returned.
	 *
	 * Reasons for why a file may fail to open are incredibly OS-specific and not exposed in this
	 * function.
	 */
	bool OpenFile(String const & filePath, FileAccess::OpenFlags openFlags, FileAccess & result);

	FileLoadError LoadFile(
		Allocator * allocator,
		String const & filePath,
		FileContents & fileContents
	);

	/**
	 * Enumerates over the files in `directoryPath`, invoking `action` for each entry with its name
	 * passed as an argument.
	 */
	uint64_t EnumerateFiles(String const & directoryPath, Callable<void(String const &)> action);

	/**
	 * Interface to a dynamic binary of code loaded at runtime.
	 */
	struct Library {
		void * handle;

		/**
		 * Attempts to find a symbol named `symbol` within the loaded code, returning `nullptr` if no
		 * matching symbol was found.
		 */
		void * FindSymbol(String const & symbol);

		/**
		 * Frees the resources associated with the `Library`.
		 */
		void Free();
	};

	/**
	 * Attempts to open the library specified at `libraryPath`.
	 *
	 * A successfully opened library will be written to `result` and `true` is returned. Otherwise,
	 * `false` is returned.
	 *
	 * Reasons for why a library may fail to open are incredibly OS-specific and not exposed in this
	 * function.
	 */
	bool OpenLibrary(String const & libraryPath, Library & result);

	/**
	 * Prints `message` to the operating system standard output.
	 */
	void Print(String const & message);

	bool PathExists(String const & path);

	String PathExtension(String const & path);

	size_t CopyMemory(Slice<uint8_t> destination, Slice<uint8_t const> const & source);

	Slice<uint8_t> WriteMemory(Slice<uint8_t> destination, uint8_t value);

	Slice<uint8_t> ZeroMemory(Slice<uint8_t> destination);

	/**
	 * Retrieves the default dynamic memory allocation strategy used by the system.
	 */
	Allocator * DefaultAllocator();

	class Object {
		public:
		Object() = default;

		Object(Object const &) = delete;

		virtual ~Object() {};

		virtual bool Equals(Object const * that) const {
			return (this == that);
		}

		virtual uint64_t ToHash() const {
			return reinterpret_cast<int64_t>(this);
		}

		virtual String ToString() const {
			return String{"{}"};
		}
	};

	class Allocator : public Object {
		public:
		virtual uint8_t * Allocate(size_t size) = 0;

		virtual void Deallocate(void * allocation) = 0;

		virtual uint8_t * Reallocate(void * allocation, size_t size) = 0;
	};

	template<typename Type> class Owned final : public Object {
		public:
		Type value;

		Owned() = default;

		Owned(Type const & value) : value{value} { }

		~Owned() override {
			this->value.Free();
		}
	};

	template<typename Type> class Array {
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
	 * An `Array` with a compile-time known fixed length.
	 *
	 * `FixedArray` benefits from using aligned memory, and as such, cannot be resized in any way.
	 */
	template<typename Type, size_t Len> class FixedArray final : public Object, public Array<Type> {
		Type buffer[Len];

		public:
		enum {
			FixedLength = Len,
		};

		/**
		 * Initializes the contents with the default values of `Type`.
		 */
		FixedArray() {
			for (size_t i = 0; i < Len; i += 1) this->buffer[i] = Type{};
		}

		/**
		 * Initializes the contents with the contents of `values`.
		 */
		FixedArray(Type (&values)[Len]) {
			for (size_t i = 0; i < Len; i += 1) this->buffer[i] = values[i];
		}

		/**
		 * Assigns the contents of `values` to the `FixedArray`.
		 */
		FixedArray(Type (&&values)[Len]) {
			for (size_t i = 0; i < Len; i += 1) this->buffer[i] = std::move(values[i]);
		}

		Type & At(size_t index) override {
			assert(index < Len);

			return this->buffer[index];
		}

		Type const & At(size_t index) const override {
			assert(index < Len);

			return this->buffer[index];
		}

		size_t Length() const override {
			return Len;
		}

		Type * Pointer() override {
			return this->buffer;
		}

		Type const * Pointer() const override {
			return this->buffer;
		}

		Slice<Type> Sliced(size_t a, size_t b) override {
			return Slice<Type>{
				.length = b,
				.pointer = (this->buffer + a)
			};
		}

		Slice<Type const> Sliced(size_t a, size_t b) const override {
			return Slice<Type const>{
				.length = b,
				.pointer = (this->buffer + a)
			};
		}

		Slice<Type> Values() override {
			return Slice<Type>{
				.length = Len,
				.pointer = this->buffer
			};
		}

		Slice<Type const> Values() const override {
			return Slice<Type const>{
				.length = Len,
				.pointer = this->buffer
			};
		}
	};

	/**
	 * An `Array` with a dynamically-sized length.
	 *
	 * `DynamicArray` trades initialization speed and buffer locality for the ability to assign and
	 * change the number of elements at runtime.
	 */
	template<typename Type> class DynamicArray final : public Object, public Array<Type> {
		Allocator * allocator;

		Slice<Type> buffer;

		public:
		/**
		 * Allocates a buffer using `allocator` of `length` elements and initializes it with the
		 * default contents of `Type`.
		 */
		DynamicArray(Allocator * allocator, size_t length) : allocator{allocator} {
			this->buffer = Slice<Type>{.pointer = reinterpret_cast<Type *>(
				this->allocator->Allocate(sizeof(Type) * length)
			)};

			if (this->buffer.pointer != nullptr) {
				this->buffer.length = length;

				// Initialize array contents.
				for (size_t i = 0; i < length; i += 1) this->buffer.At(i) = Type{};
			}
		}

		~DynamicArray() override {
			if (this->buffer.pointer) {
				this->allocator->Deallocate(this->buffer.pointer);

				// Destruct array contents.
				for (size_t i = 0; i < this->buffer.length; i += 1) this->buffer.At(i).~Type();
			}
		}

		Type & At(size_t index) override {
			return this->buffer.At(index);
		}

		Type const & At(size_t index) const override {
			return this->buffer.At(index);
		}

		size_t Length() const override {
			return this->buffer.length;
		}

		Type * Pointer() override {
			return this->buffer.pointer;
		}

		Type const * Pointer() const override {
			return this->buffer.pointer;
		}

		/**
		 * Releases ownership of the allocated buffer from the `DynamicArray` and passes it to the
		 * caller wrapped in a `Slice`.
		 *
		 * As this releases dynamic memory, the responsibility of lifetime management becomes the
		 * programmer's, as the `DynamicArray` no longer knows about it.
		 *
		 * If the `DynamicArray` contains zero elements, an empty `Slice` is returned.
		 */
		Slice<Type> Release() {
			Slice<Type> slice = this->buffer;
			this->buffer = Slice<Type>{};

			return slice;
		}

		/**
		 * Attempts to resize the `DynamicArray` buffer at runtime.
		 *
		 * A return value of `false` means that the operation failed and the `DynamicArray` now has
		 * a length of `0`.
		 */
		bool Resize(size_t length) {
			this->buffer.pointer = this->allocator->Reallocate(this->buffer.pointer, length);
			bool const allocationSucceeded = (this->buffer.pointer != nullptr);
			this->buffer.length = (length * allocationSucceeded);

			return allocationSucceeded;
		}

		Slice<Type> Sliced(size_t a, size_t b) override {
			return this->buffer.Sliced(a, b);
		}

		Slice<Type const> Sliced(size_t a, size_t b) const override {
			return this->buffer.Sliced(a, b);
		}

		Slice<Type> Values() override {
			return this->buffer;
		}

		Slice<Type const> Values() const override {
			return this->buffer;
		}
	};

	constexpr float Max(float const a, float const b) {
		return ((a > b) ? a : b);
	}

	constexpr float Min(float const a, float const b) {
		return ((a < b) ? a : b);
	}

	constexpr float Clamp(float const value, float const lower, float const upper) {
		return Max(lower, Min(value, upper));
	}

	float Floor(float const value);

	float Pow(float const value, float const exponent);

	struct Matrix {
		static constexpr size_t dimensions = 4;

		float elements[dimensions * dimensions];

		constexpr float & At(size_t row, size_t column) {
			return this->elements[column + (row * dimensions)];
		}

		static constexpr Matrix Identity() {
			Matrix identity = Matrix{};

			for (size_t i = 0; i < dimensions; i += 1) identity.At(i, i) = 1.f;

			return identity;
		}
	};

	constexpr Matrix OrthographicMatrix(
		float left,
		float right,
		float bottom,
		float top,
		float near,
		float far
	) {
		Matrix result = Matrix::Identity();
		result.elements[0 + 0 * 4] = 2.0f / (right - left);
		result.elements[1 + 1 * 4] = 2.0f / (top - bottom);
		result.elements[2 + 2 * 4] = 2.0f / (near - far);
		result.elements[3 + 0 * 4] = (left + right) / (left - right);
		result.elements[3 + 1 * 4] = (bottom + top) / (bottom - top);
		result.elements[3 + 2 * 4] = (far + near) / (far - near);

		return result;
	}

	constexpr Matrix TranslationMatrix(float x, float y, float z) {
		Matrix result = Matrix::Identity();
		result.At(0, 3) = x;
		result.At(1, 3) = y;
		result.At(2, 3) = z;

		return result;
	}

	struct Vector2 {
		float x, y;

		constexpr Vector2 Add(Vector2 const & that) const {
			return Vector2{(this->x + that.x), (this->y + that.y)};
		}

		constexpr Vector2 Sub(Vector2 const & that) const {
			return Vector2{(this->x - that.x), (this->y - that.y)};
		}

		constexpr Vector2 Mul(Vector2 const & that) const {
			return Vector2{(this->x * that.x), (this->y * that.y)};
		}

		constexpr Vector2 Div(Vector2 const & that) const {
			return Vector2{(this->x / that.x), (this->y / that.y)};
		}

		Vector2 Floor() const {
			return Vector2{Ona::Floor(this->x), Ona::Floor(this->y)};
		}

		constexpr Vector2 Normalized() const {
			return Vector2{Clamp(this->x, -1.f, 1.f), Clamp(this->y, -1.f, 1.f)};
		}
	};

	struct Vector3 {
		float x, y, z;

		constexpr Vector3 Add(Vector3 const & that) const {
			return Vector3{(this->x + that.x), (this->y + that.y), (this->z + that.z)};
		}

		constexpr Vector3 Sub(Vector3 const & that) const {
			return Vector3{(this->x - that.x), (this->y - that.y), (this->z - that.z)};
		}

		constexpr Vector3 Mul(Vector3 const & that) const {
			return Vector3{(this->x * that.x), (this->y * that.y), (this->z * that.z)};
		}

		constexpr Vector3 Div(Vector3 const & that) const {
			return Vector3{(this->x / that.x), (this->y / that.y), (this->z / that.z)};
		}
	};

	struct Vector4 {
		float x, y, z, w;

		constexpr Vector4 Add(Vector4 const & that) const {
			return Vector4{
				(this->x + that.x),
				(this->y + that.y),
				(this->z + that.z),
				(this->w + that.w)
			};
		}

		constexpr Vector4 Sub(Vector4 const & that) const {
			return Vector4{
				(this->x - that.x),
				(this->y - that.y),
				(this->z - that.z),
				(this->w - that.w)
			};
		}

		constexpr Vector4 Mul(Vector4 const & that) const {
			return Vector4{
				(this->x * that.x),
				(this->y * that.y),
				(this->z * that.z),
				(this->w * that.w)
			};
		}

		constexpr Vector4 Div(Vector4 const & that) const {
			return Vector4{
				(this->x / that.x),
				(this->y / that.y),
				(this->z / that.z),
				(this->w / that.w)
			};
		}
	};

	struct Point2 {
		int32_t x, y;
	};

	constexpr int64_t Area(Point2 dimensions) {
		return (dimensions.x * dimensions.y);
	}

	class Channel;

	void CloseChannel(Channel * & channel);

	Channel * OpenChannel(uint32_t typeSize);

	uint32_t ChannelReceive(Channel * channel, Slice<uint8_t> outputBuffer);

	uint32_t ChannelSend(Channel * channel, Slice<uint8_t const> const & inputBuffer);

	void InitScheduler();

	void ScheduleTask(Callable<void()> const & task);

	void WaitAllTasks();
}

#include "common/collections.hpp"

#endif
