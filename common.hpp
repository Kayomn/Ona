#ifndef ONA_COMMON_H
#define ONA_COMMON_H

#include <cassert>
#include <stddef.h>
#include <stdint.h>
#include <new>
#include <initializer_list>

#define $packed __attribute__ ((packed))

#define $length(arr) (sizeof(arr) / sizeof(*arr))

#include "common/core.hpp"
#include "common/text.hpp"
#include "common/math.hpp"

namespace Ona {
	/**
	 * Opaque handle to a file-like system resource.
	 */
	class SystemStream final : public Stream {
		private:
		String systemPath;

		uint64_t handle;

		public:
		SystemStream() = default;

		~SystemStream() override;

		/**
		 * Seeks the available number of bytes that the stream currently contains.
		 *
		 * Reading from and writing to the stream may modify the result, so it is advised that it be
		 * used in a loop in case any further bytes become available after an initial read.
		 */
		uint64_t AvailableBytes() override;

		/**
		 * Attempts to close the stream, silently failing if there is nothing to close.
		 */
		void Close();

		/**
		 * Retrieves the system path representing the stream.
		 */
		String ID() override;

		/**
		 * Attempts to open a stream on `systemPath` with the access rules defined by `openFlags`,
		 * returning `true` if the stream was successful opened and `false` if it was not.
		 */
		bool Open(String const & systemPath, Stream::OpenFlags openFlags);

		/**
		 * Attempts to read as many bytes as will fit in `input` from the stream, returning the
		 * number of bytes actually read.
		 */
		uint64_t ReadBytes(Slice<uint8_t> input) override;

		/**
		 * Attempts to read as many UTF-8 characters as will fit in `input` from the stream,
		 * returning the number of bytes actually read.
		 */
		uint64_t ReadUtf8(Slice<char> input) override;

		/**
		 * Repositions the stream cursor to `offset` bytes relative to the beginning of the stream.
		 */
		int64_t SeekHead(int64_t offset) override;

		/**
		 * Repositions the stream cursor to `offset` bytes relative to the end of the stream.
		 */
		int64_t SeekTail(int64_t offset) override;

		/**
		 * Moves the cursor `offset` bytes from its current position.
		 */
		int64_t Skip(int64_t offset) override;

		/**
		 * Attempts to write as many bytes to the stream as there are in `output`, returning the
		 * number of bytes actually written.
		 */
		uint64_t WriteBytes(Slice<uint8_t const> const & output) override;
	};

	/**
	 * Enumerates over the data located at `systemPath`, invoking `action` for each entry with its
	 * name passed as an argument.
	 */
	uint32_t EnumeratePath(String const & systemPath, Callable<void(String const &)> action);

	/**
	 * Prints `message` to the system standard output.
	 */
	void Print(String const & message);

	/**
	 * Checks if `systemPath` is accessible to the process, returning `true` if it does and `false`
	 * if it doesn't.
	 */
	bool PathExists(String const & systemPath);

	/**
	 * Retrieves the last period-delimited component of `systemPath`, returning the whole of
	 * `systemPath` if it does not contain any periods.
	 */
	String PathExtension(String const & systemPath);

	/**
	 * Copies the literal byte-formatted memory contents of `source` into `destination`, returning
	 * the number of bytes copied.
	 *
	 * `CopyMemory` will choose the minimum length between `destination` and `source` to avoid
	 * overrunning either buffer.
	 */
	size_t CopyMemory(Slice<uint8_t> destination, Slice<uint8_t const> const & source);

	/**
	 * Writes the byte `value` to every byte in `destination`, returning the buffer for chaining.
	 */
	Slice<uint8_t> WriteMemory(Slice<uint8_t> destination, uint8_t value);

	/**
	 * Writes `0` to every byte in `destination`, returning the buffer for chaining.
	 */
	Slice<uint8_t> ZeroMemory(Slice<uint8_t> destination);

	/**
	 * Retrieves the default dynamic memory allocation strategy used by the system.
	 */
	Allocator * DefaultAllocator();

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
	 * An `Array` with a compile-time known fixed length.
	 *
	 * `FixedArray` benefits from using aligned memory, and as such, cannot be resized in any way.
	 */
	template<typename Type, size_t Len> class FixedArray final : public Array<Type> {
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
	template<typename Type> class DynamicArray final : public Array<Type> {
		Allocator * allocator;

		Slice<Type> buffer;

		public:
		/**
		 * Assigns `allocator` to a zero-length array of `Type` for use when resizing.
		 */
		DynamicArray(Allocator * allocator) : allocator{allocator}, buffer{} {

		}

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
			this->buffer.pointer = reinterpret_cast<Type *>(
				this->allocator->Reallocate(this->buffer.pointer, sizeof(Type) * length)
			);

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
