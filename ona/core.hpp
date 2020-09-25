#ifndef CORE_H
#define CORE_H

#include <stddef.h>
#include <stdint.h>
#include <type_traits>
#include <new>
#include <cmath>

#define extends : public

#define let auto

namespace Ona::Core {
	/**
	 * Assertion function used to abort the process if `expression` does not evaluate to `true`,
	 * with `message` as the error message.
	 *
	 * This function may be optimized out when optimization flags are used.
	 */
	void Assert(bool expression, char const * message);

	struct Matrix {
		float elements[4 * 4];
	};

	struct Vector2 {
		float x, y;

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
			return Slice<Casted<CastType>>::Of(
				reinterpret_cast<Casted<CastType> *>(this->pointer),
				(this->length * sizeof(Type))
			);
		}

		/**
		 * Reinterprets the `Slice` as a `Slice` of raw, unsigned bytes.
		 */
		constexpr Slice<Casted<uint8_t>> AsBytes() const {
			return Slice<Casted<uint8_t>>::Of(
				reinterpret_cast<Casted<uint8_t> *>(this->pointer),
				(this->length * sizeof(Type))
			);
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
		 * Creates a `Slice` from `pointer` that provides a view into the memory for `length`
		 * elements.
		 */
		static constexpr Slice Of(Type * pointer, size_t const length) {
			return Slice{length, pointer};
		}

		/**
		 * Creates a new `Slice` from the `Slice`, granting access to elements from index `a` to
		 * position `b`.
		 */
		constexpr Slice Sliced(size_t a, size_t b) {
			return Slice::Of((this->pointer + a), b);
		}

		/**
		 * Creates a new `Slice` of `const` `Type` from the `Slice`, granting access to elements
		 * from index `a` to position `b`.
		 */
		constexpr Slice<Type const> Sliced(size_t a, size_t b) const {
			return Slice<Type const>::Of((this->pointer + a), b);
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
			Assert((index < this->length), "Index out of range");

			return this->pointer[index];
		}

		/**
		 * Provides non-mutable access to the value at index `index` of the `Slice.
		 */
		constexpr Type const & operator()(size_t index) const {
			Assert((index < this->length), "Index out of range");

			return this->pointer[index];
		}

		/**
		 * Checks for member-wise equality between the `Slice` and `that`.
		 */
		constexpr bool operator==(Slice const & that) const {
			return ((this->pointer == that.pointer) && (this->length == that.length));
		}

		/**
		 * Checks for member-wise inequality between the `Slice` and `that`.
		 */
		constexpr bool operator!=(Slice const & that) const {
			return ((this->pointer != that.pointer) && (this->length != that.length));
		}

		/**
		 * Provides casting to a version of `Slice` with a `const` `Type`.
		 */
		constexpr operator Slice<Type const>() const {
			return (*reinterpret_cast<Slice<Type const> const *>(this));
		}

		/**
		 * Provides casting to a `bool` value based on whether or not the `Slice` points to
		 * `nullptr` memory.
		 */
		constexpr operator bool() const {
			return (this->pointer != nullptr);
		}
	};

	template<typename ValueType, typename ErrorType> class Result final {
		static constexpr size_t storeSize = (
			(sizeof(ValueType) > sizeof(ErrorType)) ?
			sizeof(ValueType) :
			sizeof(ErrorType)
		);

		uint8_t store[storeSize];

		bool isOk;

		public:
		Result() = default;

		Result(Result const & that) {
			this->isOk = that.isOk;

			if (this->isOk) {
				this->Value() = that.Value();
			} else {
				this->Error() = that.Error();
			}
		}

		ErrorType & Error() {
			Assert((!this->isOk), "Result is ok");

			return (*reinterpret_cast<ErrorType *>(this->store));
		}

		ErrorType const & Error() const {
			Assert((!this->isOk), "Result is ok");

			return (*reinterpret_cast<ErrorType const *>(this->store));
		}

		bool IsOk() const {
			return this->isOk;
		}

		ValueType & Value() {
			Assert(this->isOk, "Result is erroneous");

			return (*reinterpret_cast<ValueType *>(this->store));
		}

		ValueType const & Value() const {
			Assert(this->isOk, "Result is erroneous");

			return (*reinterpret_cast<ValueType const *>(this->store));
		}

		static Result Ok(ValueType const & value) {
			Result result;
			result.isOk = true;

			new (result.store) ValueType{value};

			return result;
		}

		static Result Fail(ErrorType const & error) {
			Result result;
			result.isOk = false;

			new (result.store) ErrorType{error};

			return result;
		}
	};

	/**
	 * Convenience alias for a `Slice` of integer values capable of being used to encode text.
	 *
	 * `Chars` does not care about the kind of encoding used, and will therefore accept anything
	 * that can be represented as 8-bit values - most typically, ASCII and UTF-8.
	 *
	 * It should be noted that `Chars::length` is only representative of the number of bytes used by
	 * the character encoding, and may not be equivalent to the number of encoded characters in all
	 * circumstances.
	 *
	 * For more complex text manipulation, the use of `String` is recommended instead.
	 */
	using Chars = Slice<char const>;

	class Allocator {
		public:
		virtual Slice<uint8_t> Allocate(size_t size) = 0;

		virtual void Deallocate(uint8_t * allocation) = 0;

		virtual Slice<uint8_t> Reallocate(uint8_t * allocation, size_t size) = 0;

		template<typename Type, typename... Args> Type * New(Args... args) {
			uint8_t * allocation = this->Allocate(sizeof(Type)).pointer;

			new (allocation) Type{args...};

			return reinterpret_cast<Type *>(allocation);
		}
	};

	/**
	 * Copies the memory contents of `source` into the memory contents of `destination`, returning
	 * the number of bytes actually copied.
	 */
	size_t CopyMemory(Slice<uint8_t> destination, Slice<uint8_t const> source);

	template<typename Type> void WriteMemory(Slice<Type> destination, Type const & value) {
		Type * target = destination.pointer;
		Type const * boundary = (target + destination.length);

		while (target != boundary) {
			(*target) = value;
			target += 1;
		}
	}

	template<typename Type> Slice<uint8_t const> BytesOf(Type & value) {
		return Slice<Type>::Of(&value, 1).AsBytes();
	}

	/**
	 * Zeroes the memory contents of `destination`.
	 */
	void ZeroMemory(Slice<uint8_t> & destination);

	/**
	 * Creates a `Chars` from the memory contents at `pointer`.
	 *
	 * The length of the `Chars` is determined by the position of the null terminator character. As
	 * such, memory that is not zero-terminated may overrun when being read, as the function cannot
	 * determine the end of the character sequence.
	 */
	constexpr Chars CharsFrom(char const * pointer) {
		size_t length = 0;

		while (*(pointer + length)) length += 1;

		return Chars::Of(pointer, length);
	}

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
			return Slice<uint8_t const>::Of(
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

	Slice<uint8_t> Allocate(size_t size);

	void Deallocate(uint8_t * allocation);

	Slice<uint8_t> Reallocate(uint8_t * allocation, size_t size);

	template<typename Type, typename... Args> Type * New(Args... args) {
		uint8_t * allocation = Allocate(sizeof(Type)).pointer;

		new (allocation) Type{args...};

		return reinterpret_cast<Type *>(allocation);
	}

	template<typename Type> class Array final {
		Allocator * allocator;

		size_t length;

		Slice<Type> values;

		public:
		Array() = default;

		constexpr Array(Allocator * allocator) : allocator{allocator}, length{}, values{}  { }

		Array(Array const & that) {
			(*this) = Of(this->allocator, this->values);
		}

		~Array() {
			for (auto & value : this->values) {
				value.~Type();
			}

			if (this->allocator) {
				this->allocator->Deallocate(this->values.pointer);
			} else {
				Deallocate(this->values.pointer);
			}
		}

		constexpr Allocator * AllocatorOf() {
			return this->allocator;
		}

		constexpr size_t Count() const {
			return this->length;
		}

		static Array New(Allocator * allocator, size_t size) {
			Array array = {allocator};

			if (allocator) {
				array.values = allocator->Allocate(sizeof(Type) * size).template As<Type>();
			} else {
				array.values = Allocate(sizeof(Type) * size).As<Type>();
			}

			WriteMemory(array.values, Type{});

			return array;
		}

		static Array Of(Allocator * allocator, Slice<Type> const & elements) {
			Array array = {allocator};

			if (allocator) {
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

	enum class FileIoError {
		None,
		BadAccess
	};

	enum class FileOpenError {
		None,
		BadAccess,
		NotFound
	};

	enum class FileLoadError {
		None,
		NotFound,
		BadAccess,
		Resources
	};

	union FileDescriptor {
		int unixHandle;

		void* userdata;

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

		void (*closer)(FileDescriptor descriptor);

		int64_t (*seeker)(
			FileDescriptor descriptor,
			SeekBase seekBase,
			int64_t offset,
			FileIoError * error
		);

		size_t (*reader)(FileDescriptor descriptor, Slice<uint8_t> output, FileIoError * error);

		size_t (*writer)(
			FileDescriptor descriptor,
			Slice<uint8_t const> input,
			FileIoError * error
		);
	};

	struct File {
		enum OpenFlags {
			OpenUnknown = 0,
			OpenRead = 0x1,
			OpenWrite = 0x2
		};

		FileOperations const * operations;

		FileDescriptor descriptor;

		static constexpr File Bad() {
			return File{nullptr, FileDescriptor{0}};
		}

		void Free();

		void Print(String const & string, FileIoError * error);

		size_t Read(Slice<uint8_t> const & output, FileIoError * error);

		int64_t SeekHead(int64_t offset, FileIoError * error);

		int64_t SeekTail(int64_t offset, FileIoError * error);

		int64_t Skip(int64_t offset, FileIoError * error);

		int64_t Tell(FileIoError * error);

		size_t Write(Slice<uint8_t const> const & input, FileIoError * error);
	};

	File & OutFile();

	File OpenFile(String const & filePath, File::OpenFlags flags, FileOpenError * error);

	Array<uint8_t> LoadFile(Allocator * allocator, String const & filePath, FileLoadError * error);

	struct Library {
		void * context;

		void * FindSymbol(String const & symbolName);

		void Free();
	};

	enum class LibraryError {
		None,
		CantLoad
	};

	Library OpenLibrary(Chars filePath, LibraryError * error);

	struct Point2 {
		int32_t x, y;
	};

	union Color {
		static constexpr size_t channelMax = 255;

		struct {
			uint8_t r, g, b, a;
		};

		uint32_t value;

		static constexpr Color Of(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) {
			return Color{red, green, blue, alpha};
		}
	};

	constexpr Color Greyscale(uint8_t value) {
		return Color::Of(value, value, value, Color::channelMax);
	}

	constexpr Color Rgb(uint8_t red, uint8_t green, uint8_t blue) {
		return Color::Of(red, green, blue, Color::channelMax);
	}

	enum class ImageError {
		None,
		UnsupportedFormat,
		OutOfMemory
	};

	struct Image {
		Allocator * allocator;

		Point2 dimensions;

		Slice<Color> pixels;

		void Free();

		static Image From(
			Allocator * allocator,
			Point2 dimensions,
			Color * pixels,
			ImageError * error
		);

		static Image Solid(
			Allocator * allocator,
			Point2 const & dimensions,
			Color color,
			ImageError * error
		);
	};
}

#endif
