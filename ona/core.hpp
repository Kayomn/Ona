#ifndef CORE_H
#define CORE_H

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
			// Assert((index < this->length));

			return this->pointer[index];
		}

		/**
		 * Provides non-mutable access to the value at index `index` of the `Slice.
		 */
		constexpr Type const & operator()(size_t index) const {
			// Assert((index < this->length));

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
	 * Assertion function used to abort the process if `expression` does not evaluate to `true`,
	 * with `message` as the error message.
	 *
	 * This function may be optimized out when optimization flags are used.
	 */
	void Assert(bool expression, Chars const & message);

	template<typename Type> class Optional final {
		static constexpr bool isNotPointer = (!std::is_pointer<Type>::value);

		uint8_t store[sizeof(Type) + static_cast<size_t>(isNotPointer)];

		public:
		Optional() = default;

		explicit Optional(Type const & value) {
			(*reinterpret_cast<Type *>(this->store)) = value;

			if constexpr (isNotPointer) {
				this->store[sizeof(Type)] = 1;
			}
		}

		Optional(Optional const & that) {
			if (this->HasValue()) {
				(*reinterpret_cast<Type *>(this->store)) = (
					*reinterpret_cast<Type const *>(that.store)
				);
			}
		}

		bool HasValue() const {
			if constexpr (isNotPointer) {
				return static_cast<bool>(this->store[sizeof(Type)]);
			} else {
				return ((*reinterpret_cast<Type const *>(this->store)) != nullptr);
			}
		}

		Type & Value() {
			Assert(this->HasValue(), CharsFrom("Optional is empty"));

			return (*reinterpret_cast<Type *>(this->store));
		}

		Type const & Value() const {
			Assert(this->HasValue(), CharsFrom("Optional is empty"));

			return (*reinterpret_cast<Type const *>(this->store));
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

		explicit Result(ValueType const & value) : isOk{true} {
			new (this->store) ValueType{value};
		}

		Result(Result const & that) {
			this->isOk = that.isOk;

			if (this->isOk) {
				this->Value() = that.Value();
			} else {
				this->Error() = that.Error();
			}
		}

		ErrorType & Error() {
			Assert((!this->isOk), CharsFrom("Result is ok"));

			return (*reinterpret_cast<ErrorType *>(this->store));
		}

		ErrorType const & Error() const {
			Assert((!this->isOk), CharsFrom("Result is ok"));

			return (*reinterpret_cast<ErrorType const *>(this->store));
		}

		ValueType & Expect(Chars const & message) {
			Assert(this->isOk, message);

			return (*reinterpret_cast<ValueType *>(this->store));
		}

		ValueType const & Expect(Chars const & message) const {
			Assert(this->isOk, message);

			return (*reinterpret_cast<ValueType const *>(this->store));
		}

		bool IsOk() const {
			return this->isOk;
		}

		ValueType & Value() {
			return this->Expect(CharsFrom("Result is erroneous"));
		}

		ValueType const & Value() const {
			return this->Expect(CharsFrom("Result is erroneous"));
		}

		static Result Ok(ValueType const & value) {
			return Result{value};
		}

		static Result Fail(ErrorType const & error) {
			Result result;
			result.isOk = false;

			new (result.store) ErrorType{error};

			return result;
		}
	};

	class Allocator {
		public:
		virtual Slice<uint8_t> Allocate(size_t size) = 0;

		virtual void Deallocate(uint8_t * allocation) = 0;

		virtual Slice<uint8_t> Reallocate(uint8_t * allocation, size_t size) = 0;

		template<typename Type, typename... Args> Optional<Type *> New(Args... args) {
			using Opt = Optional<Type *>;
			uint8_t * allocation = this->Allocate(sizeof(Type)).pointer;

			if (allocation) {
				new (allocation) Type{args...};

				return Opt{reinterpret_cast<Type *>(allocation)};
			}

			return Opt{};
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
		return SliceOf(&value, 1).AsBytes();
	}

	/**
	 * Zeroes the memory contents of `destination`.
	 */
	void ZeroMemory(Slice<uint8_t> & destination);

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

	Slice<uint8_t> Allocate(size_t size);

	void Deallocate(uint8_t * allocation);

	Slice<uint8_t> Reallocate(uint8_t * allocation, size_t size);

	template<typename Type, typename... Args> Optional<Type *> New(Args... args) {
		using Opt = Optional<Type *>;
		uint8_t * allocation = Allocate(sizeof(Type)).pointer;

		if (allocation) {
			new (allocation) Type{args...};

			return Opt{reinterpret_cast<Type *>(allocation)};
		}

		return Opt{};
	}

	template<typename Type> class Array final {
		Optional<Allocator *> allocator;

		size_t length;

		Slice<Type> values;

		public:
		Array() = default;

		constexpr Array(Optional<Allocator *> allocator) : allocator{allocator}, length{}, values{}  { }

		Array(Array const & that) {
			(*this) = Of(this->allocator, this->values);
		}

		constexpr Optional<Allocator *> AllocatorOf() {
			return this->allocator;
		}

		constexpr size_t Count() const {
			return this->length;
		}

		void Free() {
			for (auto & value : this->values) {
				value.~Type();
			}

			if (this->allocator) {
				this->allocator.Value()->Deallocate(this->values.pointer);
			} else {
				Deallocate(this->values.pointer);
			}
		}

		static Array Init(Optional<Allocator *> allocator, size_t size) {
			Array array = {allocator};

			if (allocator.HasValue()) {
				array.values = allocator.Value()->Allocate(sizeof(Type) * size).template As<Type>();
			} else {
				array.values = Allocate(sizeof(Type) * size).As<Type>();
			}

			WriteMemory(array.values, Type{});

			return array;
		}

		static Array Of(Optional<Allocator *> allocator, Slice<Type> const & elements) {
			Array array = {allocator};

			if (allocator.HasValue()) {
				array.values = allocator.Value()->Allocate(
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

		void (*closer)(FileDescriptor descriptor);

		int64_t (*seeker)(FileDescriptor descriptor, SeekBase seekBase, int64_t offset);

		size_t (*reader)(FileDescriptor descriptor, Slice<uint8_t> output);

		size_t (*writer)(FileDescriptor descriptor, Slice<uint8_t const> input);
	};

	struct File {
		enum OpenFlags {
			OpenUnknown = 0,
			OpenRead = 0x1,
			OpenWrite = 0x2
		};

		FileOperations const * operations;

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

	Result<Array<uint8_t>, FileLoadError> LoadFile(Optional<Allocator *> allocator, String const & filePath);

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

	class Image {
		public:
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
