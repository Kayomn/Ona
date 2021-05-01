
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
	 * Attempts to allocate `size` bytes of memory using `allocator` as the allocation strategy,
	 * returning the address of the allocated bytes or `nullptr` if there is no more memory
	 * available.
	 */
	uint8_t * Allocate(Allocator allocator, size_t size);

	/**
	 * Attempts to re-allocate `size` bytes of existing memory in `allocation` using `allocator` as
	 * the allocation strategy, returning the address of the allocated bytes or `nullptr` if there
	 * is no more memory available.
	 *
	 * `Ona::Reallocate` may attempt to re-use the address referenced by `allocation` in the new
	 * allocation if it is not `nullptr`. The address referenced should always be considered invalid
	 * after the function returns, as the runtime will write `nullptr` to it regardless.
	 *
	 * If `allocation` is `nullptr`, `Ona::Reallocate` works identically to `Ona::Allocate`.
	 */
	uint8_t * Reallocate(Allocator allocator, void * allocation, size_t size);

	/**
	 * Attempts to the deallocate the memory in `allocation` using `allocator`. If `allocation` was
	 * not allocated by `allocator`, undefined behavior will occur.
	 */
	void Deallocate(Allocator allocator, void * allocation);
}
