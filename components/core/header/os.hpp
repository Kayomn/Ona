
namespace Ona::Engine {
	using namespace Ona::Core;

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
	struct File {
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

		File() = default;

		File(
			FileOperations const * operations,
			void * handle
		) : operations{operations}, handle{handle} {

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

	/**
	 * Checks that the file at `filePath` is available to be opened.
	 *
	 * Reasons for why a file may not be openable are incredibly OS-specific and not exposed in this
	 * function.
	 */
	bool CheckFile(String const & filePath);

	/**
	 * Attempts to open the file specified at `filePath` using `openFlags`.
	 *
	 * A successfully opened file will be written to `result` and `true` is returned. Otherwise,
	 * `false` is returned.
	 *
	 * Reasons for why a file may fail to open are incredibly OS-specific and not exposed in this
	 * function.
	 */
	bool OpenFile(String const & filePath, File::OpenFlags openFlags, File & result);

	/**
	 * Interface to a dynamic binary of code loaded at runtime.
	 */
	struct Library {
		void * handle;

		/**
		 * Attempts to find a symbol named `symbol` within the loaded code, returning `nullptr` if
		 * no matching symbol was found.
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
	 * Resource for acquiring mutually exclusive (locking) access to a section of code for a
	 * programmatically determined amount of time.
	 */
	struct Mutex {
		void * handle;

		/**
		 * Frees the resources associated with the `Mutex`.
		 */
		void Free();

		/**
		 * Locks the region of code it is called in, preventing any other concurrent code from
		 * proceding past it.
		 */
		void Lock();

		/**
		 * Ends the locked state, allowing concurrent code to continue past the region it had
		 * locked.
		 */
		void Unlock();
	};

	enum class MutexError {
		None,
		OSFailure,
		OutOfMemory,
		ResourceLimit,
	};

	/**
	 * Attempts to create a mutex.
	 *
	 * A successfully created mutex will be written to `result` and `MutexError::None` is returned.
	 * Otherwise:
	 *
	 * `MutexError::OutOfMemory` occurs when the OS failed to allocate the memory necessary to
	 * create the mutex.
	 *
	 * `MutexError::ResourceLimit` occurs when the operating system and / or hardware has either
	 * a hard or soft limit on mutexes and the request has failed because it has been reached.
	 *
	 * `MutexError::OSFailure` is an ambiguous catch-all error state that covers any implementation-
	 * specific errors on the part of the operating system. These should be considered
	 * unrecoverable.
	 */
	MutexError CreateMutex(Mutex & result);

	/**
	 * TODO(Kayomn): Document.
	 */
	struct Condition {
		void * handle;

		/**
		 * TODO(Kayomn): Document.
		 */
		void Free();

		/**
		 * TODO(Kayomn): Document.
		 */
		void Signal();

		/**
		 * TODO(Kayomn): Document.
		 */
		void Wait(Mutex & mutex);
	};

	enum class ConditionError {
		None,
		OSFailure,
		OutOfMemory,
		ResourceLimit,
	};

	/**
	 * Attempts to create a thread condition variable.
	 *
	 * A successfully created condition will be written to `result` and `ConditionError::None` is
	 * returned. Otherwise:
	 *
	 * `ConditionError::OutOfMemory` occurs when the OS failed to allocate the memory necessary to
	 * create the condition.
	 *
	 * `ConditionError::ResourceLimit` occurs when the operating system and / or hardware has either
	 * a hard or soft limit on conditions and the request has failed because it has been reached.
	 *
	 * `ConditionError::OSFailure` is an ambiguous catch-all error state that covers any
	 * implementation- specific errors on the part of the operating system. These should be
	 * considered unrecoverable.
	 */
	ConditionError CreateCondition(Condition & result);

	/**
	 * Handle to a concurrent agent used for processing things in parallel.
	 */
	struct Thread {
		void * handle;

		/**
		 * Attempts to cancel operation of the thread, terminating it immediately.
		 *
		 * A request to cancel is just that - a request, and will also require that the thread to
		 * have been initialized with `ThreadProperties::isCancellable` as `true` for it to have any
		 * chance of working. Otherwise, it will succeede in most cases within control of the
		 * process.
		 *
		 * Successful termination will exit the thread and return `true`. Otherwise, `false` is
		 * returned.
		 */
		bool Cancel();
	};

	enum class ThreadError {
		None,
		OSFailure,
		EmptyAction,
		ResourceLimit,
	};

	/**
	 * Properties for dictating the rules of a thread.
	 */
	struct ThreadProperties {
		bool isCancellable;
	};

	/**
	 * Attempts to acquire a thread instance with the rules specified in `properties`.
	 *
	 * Some operating systems have a hard limit for the length of `name`. If this is exceded, it
	 * will simply be truncated on the tail.
	 *
	 * A successfully acquired thread will start executing `action`, be assigned the name specified
	 * in `name`, have the result written to `result`, and returns `ThreadError::None`. Otherwise:
	 *
	 * `ThreadError::EmptyAction` occurs when `action` does not have any kind of callable operation
	 * assigned to it.
	 *
	 * `ThreadError::ResourceLimit` occurs when the operating system and / or hardware has either
	 * a hard or soft limit on threads and the request has failed because it has been reached.
	 *
	 * `ThreadError::OSFailure` is an ambiguous catch-all error state that covers any
	 * implementation- specific errors on the part of the operating system. These should be
	 * considered unrecoverable.
	 */
	ThreadError AcquireThread(
		String const & name,
		ThreadProperties const & properties,
		Callable<void()> const & action,
		Thread & result
	);

	/**
	 * Returns the number of hardware-mappable contexts for executing code concurrently. On some
	 * platforms, this may be hardware threads, while on others it may be cores.
	 */
	uint32_t CountHardwareConcurrency();

	/**
	 * Prints `message` to the operating system standard output.
	 */
	void Print(String const & message);
}
