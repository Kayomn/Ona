#ifndef CORE_H
#define CORE_H

#include <cassert>
#include <stddef.h>
#include <stdint.h>
#include <type_traits>
#include <new>
#include <cmath>

#define internal static

namespace Ona::Core {
	constexpr float Max(float const a, float const b) {
		return ((a > b) ? a : b);
	}

	constexpr float Min(float const a, float const b) {
		return ((a < b) ? a : b);
	}

	constexpr float Clamp(float const value, float const lower, float const upper) {
		return Max(lower, Min(value, upper));
	}

	constexpr float Floor(float const value) {
		return std::floor(value);
	}

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

		constexpr Vector2 Floor() const {
			return Vector2{Ona::Core::Floor(this->x), Ona::Core::Floor(this->y)};
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
		 * Creates a new `Slice` from the `Slice`, granting access to elements from index `a` to
		 * position `b`.
		 */
		constexpr Slice Sliced(size_t a, size_t b) {
			return Slice{
				.length = b,
				.pointer = (this->pointer + a)
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

	template<typename> struct Callable;

	template<typename Return, typename... Args> struct Callable<Return(Args...)> {
		private:
		class Context {
			public:
			virtual Return Invoke(Args const &... args) = 0;
		};

		enum { BufferSize = 24 };

		Context * context;

		uint8_t buffer[BufferSize];

		public:
		Callable() = default;

		template<typename Type> Callable(Type const & functor) {
			class Functor : public Context {
				private:
				Type functor;

				public:
				Functor(Type const & functor) : functor{functor} { }

				Return Invoke(Args const &... args) override {
					return this->functor(args...);
				};
			};

			static_assert((sizeof(Type) <= BufferSize), "Functor cannot be larger than buffer");

			this->context = new (buffer) Functor{functor};
		}

		Return Invoke(Args const &... args) const {
			return this->context->Invoke(args...);
		}
	};

	using Chars = Slice<char const>;

	constexpr Chars CharsFrom(char const * pointer) {
		size_t length = 0;

		while (*(pointer + length)) length += 1;

		return SliceOf(pointer, length);
	}

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

	size_t CopyMemory(Slice<uint8_t> destination, Slice<uint8_t const> source);

	Slice<uint8_t> WriteMemory(Slice<uint8_t> destination, uint8_t value);

	template<typename Type> Slice<uint8_t const> AsBytes(Type & value) {
		return SliceOf(&value, 1).AsBytes();
	}

	/**
	 * Zeroes the memory contents of `destination`.
	 */
	Slice<uint8_t> ZeroMemory(Slice<uint8_t> destination);

	/**
	 * Reference-counted, UTF-8-encoded character sequence.
	 */
	struct String {
		private:
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

		constexpr bool IsDynamic() const {
			return (this->size > staticBufferSize);
		}

		constexpr size_t Length() const {
			return this->length;
		}

		static String Sentineled(String const & string);

		uint64_t ToHash() const;
	};

	class Object {
		public:
		virtual ~Object() {};

		virtual bool IsInitialized() {
			return true;
		}

		virtual bool Equals(Object const * that) const {
			return (this == that);
		}

		virtual uint64_t ToHash() const {
			return reinterpret_cast<int64_t>(this);
		}

		virtual String ToString() const {
			return String::From("{}");
		}
	};

	class Allocator {
		public:
		virtual Slice<uint8_t> Allocate(size_t size) = 0;

		virtual void Deallocate(void * allocation) = 0;

		virtual Slice<uint8_t> Reallocate(void * allocation, size_t size) = 0;

		template<typename Type, typename... Args> Type * New(Args const &... args) {
			Type * instance = reinterpret_cast<Type *>(
				ZeroMemory(this->Allocate(sizeof(Type))).pointer
			);

			if (instance) {
				new (instance) Type{args...};

				if constexpr (std::is_base_of_v<Object, Type>) {
					if (!instance->IsInitialized()) {
						this->Destroy(instance);

						instance = nullptr;
					}
				}
			}

			return instance;
		}

		template<typename Type> void Destroy(Type * & object) {
			object->~Type();
			this->Deallocate(object);

			object = nullptr;
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

		void(*closer)(FileDescriptor descriptor);

		int64_t(*seeker)(FileDescriptor descriptor, SeekBase seekBase, int64_t offset);

		size_t(*reader)(FileDescriptor descriptor, Slice<uint8_t> output);

		size_t(*writer)(FileDescriptor descriptor, Slice<uint8_t const> input);
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

	File OutFile();

	Allocator * DefaultAllocator();

	Result<File, FileOpenError> OpenFile(String const & filePath, File::OpenFlags flags);

	struct Library {
		void * context;

		void * FindSymbol(String const & symbolName);

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

	constexpr Color Greyscale(uint8_t const value) {
		return Color{value, value, value, 0xFF};
	}

	constexpr Color RGB(uint8_t const red, uint8_t const green, uint8_t const blue) {
		return Color{red, green, blue, 0xFF};
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

		static Result<Image, ImageError> From(
			Allocator * allocator,
			Point2 dimensions,
			Color * pixels
		);

		static Result<Image, ImageError> Solid(
			Allocator * allocator,
			Point2 dimensions,
			Color color
		);
	};
}

#endif
