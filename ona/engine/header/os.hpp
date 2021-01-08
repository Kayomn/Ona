
namespace Ona::Engine {
	struct Mutex;

	Mutex * AllocateMutex();

	void DestroyMutex(Mutex * & mutex);

	void LockMutex(Mutex * mutex);

	void UnlockMutex(Mutex * mutex);

	struct Condition;

	Condition * AllocateCondition();

	void DestroyCondition(Condition * & condition);

	void SignalCondition(Condition * condition);

	void WaitCondition(Condition * condition, Mutex * mutex);

	struct ThreadProperties {
		bool isCancellable;
	};

	using ThreadID = uint64_t;

	struct ThreadHandle {
		ThreadID id;

		bool(* canceller)(ThreadID id);

		bool Cancel();
	};

	uint32_t CountThreads();

	ThreadHandle AcquireThread(
		String const & name,
		ThreadProperties const & properties,
		Callable<void()> const & action
	);

	/**
	 * Attempts to load access to the operating system filesystem as a `FileServer`.
	 */
	FileServer * LoadFilesystem();

	/**
	 * Prints `message` to the operating system standard output.
	 */
	void Print(String const & message);
}
