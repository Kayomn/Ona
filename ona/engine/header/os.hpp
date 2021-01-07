
namespace Ona::Engine {
	class Mutex : public Object {
		public:
		virtual void Lock() = 0;

		virtual void Unlock() = 0;
	};

	Mutex * AllocateMutex();

	void DestroyMutex(Mutex * & mutex);

	class Condition : public Object {
		public:
		virtual void Signal() = 0;

		virtual void Wait(Mutex * mutex) = 0;
	};

	Condition * AllocateCondition();

	void DestroyCondition(Condition * & condition);

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
