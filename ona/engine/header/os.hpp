
namespace Ona::Engine {
	using namespace Ona::Core;

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
	 * Attempts to load the library specified at `libraryPath` into memory.
	 *
	 * A successfully loaded library will be written to `result` and `true` is returned. Otherwise,
	 * `false` is returned.
	 */
	bool LoadLibrary(String const & libraryPath, Library & result);

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
		OSFailure,
		OutOfMemory,
		ResourceLimit,
	};

	/**
	 * Attempts to create a mutex.
	 *
	 * A successfully created mutex will be written to `result` and an empty error is returned.
	 * Otherwise, an error is returned.
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
	Error<MutexError> CreateMutex(Mutex & result);

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
		OSFailure,
		OutOfMemory,
		ResourceLimit,
	};

	/**
	 * Attempts to create a thread condition variable.
	 *
	 * A successfully created condition will be written to `result` and an empty error is returned.
	 * Otherwise, an error is returned.
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
	Error<ConditionError> CreateCondition(Condition & result);

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
	 * A successfully acquired thread will start executing `action`, be assigned the name specified
	 * in `name`, have the result written to `result`, and return an empty `Error`. Otherwise, an
	 * error is returned.
	 *
	 * Some operating systems have a hard limit for the length of `name`. If this is exceded, it
	 * will simply be truncated on the tail.
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
	Error<ThreadError> AcquireThread(
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
	 * Attempts to load access to the operating system filesystem as a `FileServer`.
	 */
	FileServer * LoadFilesystem();

	/**
	 * Prints `message` to the operating system standard output.
	 */
	void Print(String const & message);
}
