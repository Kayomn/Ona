#ifndef CORE_H
#define CORE_H

#include <cassert>
#include <stddef.h>
#include <stdint.h>
#include <type_traits>
#include <new>
#include <cmath>

#define extends : public

#define let auto

#define internal static

namespace Ona::Core {
	struct Matrix {
		float elements[4 * 4];
	};

	struct Vector2 {
		float x, y;

		constexpr auto operator<=>(Vector2 const &) const = default;

		constexpr Vector2 operator+(Vector2 const & that) const {
			return Vector2{(this->x + that.x), (this->y + that.y)};
		}

		constexpr Vector2 operator-(Vector2 const & that) const {
			return Vector2{(this->x - that.x), (this->y - that.y)};
		}

		constexpr Vector2 operator*(Vector2 const & that) const {
			return Vector2{(this->x * that.x), (this->y * that.y)};
		}

		constexpr Vector2 operator/(Vector2 const & that) const {
			return Vector2{(this->x / that.x), (this->y / that.y)};
		}
	};

	struct Vector3 {
		float x, y, z;

		constexpr auto operator<=>(Vector3 const &) const = default;

		constexpr Vector3 operator+(Vector3 const & that) const {
			return Vector3{(this->x + that.x), (this->y + that.y), (this->z + that.z)};
		}

		constexpr Vector3 operator-(Vector3 const & that) const {
			return Vector3{(this->x - that.x), (this->y - that.y), (this->z - that.z)};
		}

		constexpr Vector3 operator*(Vector3 const & that) const {
			return Vector3{(this->x * that.x), (this->y * that.y), (this->z * that.z)};
		}

		constexpr Vector3 operator/(Vector3 const & that) const {
			return Vector3{(this->x / that.x), (this->y / that.y), (this->z / that.z)};
		}
	};

	struct Vector4 {
		float x, y, z, w;

		constexpr auto operator<=>(Vector4 const &) const = default;

		constexpr Vector4 operator+(Vector4 const & that) const {
			return Vector4{
				(this->x + that.x),
				(this->y + that.y),
				(this->z + that.z),
				(this->w + that.w)
			};
		}

		constexpr Vector4 operator-(Vector4 const & that) const {
			return Vector4{
				(this->x - that.x),
				(this->y - that.y),
				(this->z - that.z),
				(this->w - that.w)
			};
		}

		constexpr Vector4 operator*(Vector4 const & that) const {
			return Vector4{
				(this->x * that.x),
				(this->y * that.y),
				(this->z * that.z),
				(this->w * that.w)
			};
		}

		constexpr Vector4 operator/(Vector4 const & that) const {
			return Vector4{
				(this->x / that.x),
				(this->y / that.y),
				(this->z / that.z),
				(this->w / that.w)
			};
		}
	};

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

		template<typename CastType> Slice<Casted<CastType>> As() const {
			using ToType = Casted<CastType>;

			return Slice<ToType>{
				(this->length / sizeof(ToType)),
				reinterpret_cast<ToType *>(this->pointer)
			};
		}

		/**
		 * Reinterprets the `Slice` as a `Slice` of raw, unsigned bytes.
		 */
		constexpr Slice<Casted<uint8_t>> AsBytes() const {
			return Slice<Casted<uint8_t>>{
				(this->length * sizeof(Type)),
				reinterpret_cast<Casted<uint8_t> *>(this->pointer)
			};
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
		 * Returns `true` if the `Slice` references data, otherwise `false`.
		 *
		 * Note that a `Slice` may have a length of `0` but still reference data.
		 */
		constexpr bool HasValue() const {
			return (this->pointer != nullptr);
		}

		/**
		 * Creates a new `Slice` from the `Slice`, granting access to elements from index `a` to
		 * position `b`.
		 */
		constexpr Slice Sliced(size_t a, size_t b) {
			return Slice{
				b,
				(this->pointer + a)
			};
		}

		/**
		 * Creates a new `Slice` of `const` `Type` from the `Slice`, granting access to elements
		 * from index `a` to position `b`.
		 */
		constexpr Slice<Type const> Sliced(size_t a, size_t b) const {
			return SliceOf((this->pointer + a), b);
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
		 * Provides mutable access to the value at index `index` of the `Slice.
		 */
		constexpr Type & operator()(size_t index) {
			assert((index < this->length));

			return this->pointer[index];
		}

		/**
		 * Provides non-mutable access to the value at index `index` of the `Slice.
		 */
		constexpr Type const & operator()(size_t index) const {
			assert((index < this->length));

			return this->pointer[index];
		}

		constexpr auto operator<=>(Slice const &) const = default;

		/**
		 * Provides casting to a version of `Slice` with a `const` `Type`.
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

	template<typename Type> class Optional final {
		static constexpr bool isNotPointer = (!std::is_pointer<Type>::value);

		uint8_t store[sizeof(Type) + static_cast<size_t>(isNotPointer)];

		public:
		Optional() = default;

		Optional(Type const & value) {
			(*reinterpret_cast<Type *>(this->store)) = value;

			if constexpr (isNotPointer) {
				this->store[sizeof(Type)] = 1;
			}
		}

		Optional(Optional const & that) {
			(*reinterpret_cast<Type *>(this->store)) = (that.HasValue() ? that.Value() : Type{});
		}

		bool HasValue() const {
			if constexpr (isNotPointer) {
				return static_cast<bool>(this->store[sizeof(Type)]);
			} else {
				return ((*reinterpret_cast<Type const *>(this->store)) != nullptr);
			}
		}

		Type & Value() {
			assert(this->HasValue());

			return (*reinterpret_cast<Type *>(this->store));
		}

		Type const & Value() const {
			assert(this->HasValue());

			return (*reinterpret_cast<Type const *>(this->store));
		}

		Type & operator->() {
			return this->Value();
		}

		Type const & operator->() const {
			return this->Value();
		}
	};

	template<typename Type> constexpr Optional<Type> nil = {};

	template<typename ValueType, typename ErrorType> class Result final {
		static constexpr size_t typeSize = std::max(sizeof(ValueType), sizeof(ErrorType));

		uint8_t store[typeSize + 1];

		public:
		Result() = default;

		Result(Result const & that) {
			this->store[typeSize] = that.store[typeSize];

			if (this->store[typeSize]) {
				this->Value() = that.Value();
			} else {
				this->Error() = that.Error();
			}
		}

		~Result() {
			if constexpr (std::is_destructible_v<ValueType>) {
				if (this->IsOk()) {
					this->Value().~ValueType();

					return;
				}
			}

			if constexpr (std::is_destructible_v<ErrorType>) {
				this->Error().~ErrorType();
			}
		}

		ErrorType & Error() {
			assert((!this->store[typeSize]) && "Result is ok");

			return (*reinterpret_cast<ErrorType *>(this->store));
		}

		ErrorType const & Error() const {
			assert((!this->store[typeSize]) && "Result is ok");

			return (*reinterpret_cast<ErrorType const *>(this->store));
		}

		ValueType & Expect(Chars const & message) {
			assert(this->store[typeSize] && message.pointer);

			return (*reinterpret_cast<ValueType *>(this->store));
		}

		ValueType const & Expect(Chars const & message) const {
			assert(this->store[typeSize] && message.pointer);

			return (*reinterpret_cast<ValueType const *>(this->store));
		}

		bool IsOk() const {
			return static_cast<bool>(this->store[typeSize]);
		}

		ValueType & Value() {
			return this->Expect(CharsFrom("Result is erroneous"));
		}

		ValueType const & Value() const {
			return this->Expect(CharsFrom("Result is erroneous"));
		}

		static Result Ok(ValueType const & value) {
			Result result = {};
			result.store[typeSize] = 1;

			new (result.store) ValueType{value};

			return result;
		}

		static Result Fail(ErrorType const & error) {
			Result result = {};

			new (result.store) ErrorType{error};

			return result;
		}
	};

	class Allocator {
		public:
		virtual Slice<uint8_t> Allocate(size_t size) = 0;

		virtual void Deallocate(void * allocation) = 0;

		virtual Slice<uint8_t> Reallocate(void * allocation, size_t size) = 0;
	};

	Slice<uint8_t> Allocate(size_t size);

	void Deallocate(void * allocation);

	Slice<uint8_t> Reallocate(void * allocation, size_t size);

	size_t CopyMemory(Slice<uint8_t> destination, Slice<uint8_t const> source);

	void WriteMemory(Slice<uint8_t> destination, uint8_t value);

	template<typename Type> Slice<uint8_t const> AsBytes(Type & value) {
		return SliceOf(&value, 1).AsBytes();
	}

	/**
	 * Zeroes the memory contents of `destination`.
	 */
	void ZeroMemory(Slice<uint8_t> destination);

	/**
	 * Reference-counted, UTF-8-encoded character sequence.
	 */
	class String final {
		static constexpr size_t staticBufferSize = 24;

		uint32_t size;

		uint32_t length;

		union {
			uint8_t * dynamic;

			uint8_t static_[staticBufferSize];
		} buffer;

		Slice<uint8_t> CreateBuffer(size_t size);

		public:
		String() = default;

		String(String const & that);

		~String();

		Chars AsChars() const;

		constexpr Slice<uint8_t const> AsBytes() const {
			return SliceOf(
				(this->IsDynamic() ? this->buffer.dynamic : this->buffer.static_),
				this->size
			);
		}

		bool Equals(String const & that) const;

		static String From(char const * data);

		static String From(Chars const & data);

		static String Sentineled(String const & string);

		constexpr bool IsDynamic() const {
			return (this->size > staticBufferSize);
		}

		constexpr size_t Length() const {
			return this->length;
		}
	};

	template<typename Type> struct Array {
		private:
		Optional<Allocator *> allocator;

		Slice<Type> values;

		public:
		constexpr Optional<Allocator *> AllocatorOf() {
			return this->allocator;
		}

		constexpr size_t Count() const {
			return this->values.length;
		}

		static Array Init(Optional<Allocator *> allocator, size_t size) {
			Array array;
			array.allocator = allocator;

			if (allocator.HasValue()) {
				array.values = allocator->Allocate(sizeof(Type) * size).template As<Type>();
			} else {
				array.values = Allocate(sizeof(Type) * size).As<Type>();
			}

			size *= static_cast<size_t>(array.values.HasValue());

			new (array.values.pointer) Type[size];

			return array;
		}

		void Free() {
			for (let & value : this->values) value.~Type();

			if (this->allocator.HasValue()) {
				this->allocator.Value()->Deallocate(this->values.pointer);
			} else {
				Deallocate(this->values.pointer);
			}
		}

		static Array Of(Optional<Allocator *> allocator, Slice<Type> const & elements) {
			Array array = {allocator};

			if (allocator.HasValue()) {
				array.values = allocator->Allocate(
					sizeof(Type) * elements.length
				).template As<Type>();
			} else {
				array.values = Allocate(sizeof(Type) * elements.length).template As<Type>();
			}

			CopyMemory(array.values, elements);

			return array;
		}

		constexpr Slice<Type> Values() const {
			return this->values;
		}
	};

	bool CheckFile(String const & filePath);

	enum class FileOpenError {
		BadAccess,
		NotFound
	};

	enum class FileLoadError {
		NotFound,
		BadAccess,
		Resources
	};

	union FileDescriptor {
		int unixHandle;

		void * userdata;

		void Clear();

		constexpr bool IsEmpty() {
			return (this->userdata != nullptr);
		}
	};

	struct FileOperations {
		enum SeekBase {
			SeekBaseHead,
			SeekBaseCurrent,
			SeekBaseTail
		};

		using Closer = Optional<void (*)(FileDescriptor descriptor)>;

		using Seeker = Optional<
			int64_t (*)(FileDescriptor descriptor, SeekBase seekBase, int64_t offset)
		>;

		using Reader = Optional<size_t (*)(FileDescriptor descriptor, Slice<uint8_t> output)>;

		using Writer = Optional<size_t (*)(FileDescriptor descriptor, Slice<uint8_t const> input)>;

		Closer closer;

		Seeker seeker;

		Reader reader;

		Writer writer;
	};

	struct File {
		enum OpenFlags {
			OpenUnknown = 0,
			OpenRead = 0x1,
			OpenWrite = 0x2
		};

		Optional<FileOperations const *> operations;

		FileDescriptor descriptor;

		void Free();

		void Print(String const & string);

		size_t Read(Slice<uint8_t> const & output);

		int64_t SeekHead(int64_t offset);

		int64_t SeekTail(int64_t offset);

		int64_t Skip(int64_t offset);

		int64_t Tell();

		size_t Write(Slice<uint8_t const> const & input);
	};

	File & OutFile();

	Result<File, FileOpenError> OpenFile(String const & filePath, File::OpenFlags flags);

	Result<Array<uint8_t>, FileLoadError> LoadFile(
		Optional<Allocator *> allocator,
		String const & filePath
	);

	struct Library {
		void * context;

		Optional<void *> FindSymbol(String const & symbolName);

		void Free();
	};

	enum class LibraryError {
		None,
		CantLoad
	};

	Result<Library, LibraryError> OpenLibrary(Chars filePath);

	struct Point2 {
		int32_t x, y;
	};

	struct Color {
		uint8_t r, g, b, a;
	};

	constexpr let colorWhite = Color{0xFF, 0xFF, 0xFF, 0xFF};

	constexpr Color Greyscale(uint8_t const value) {
		return Color{value, value, value, 0xFF};
	}

	constexpr Color Rgb(uint8_t const red, uint8_t const green, uint8_t const blue) {
		return Color{red, green, blue, 0xFF};
	}

	enum class ImageError {
		None,
		UnsupportedFormat,
		OutOfMemory
	};

	struct Image {
		Optional<Allocator *> allocator;

		Point2 dimensions;

		Slice<Color> pixels;

		void Free();

		static Result<Image, ImageError> From(
			Optional<Allocator *> allocator,
			Point2 dimensions,
			Color * pixels
		);

		static Result<Image, ImageError> Solid(
			Optional<Allocator *> allocator,
			Point2 dimensions,
			Color color
		);
	};
}

#endif
