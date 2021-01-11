
namespace Ona::Engine {
	using namespace Ona::Collections;

	using Task = Callable<void()>;

	/**
	 * Asynchronous task scheduler backed by `Thread`.
	 */
	class Async : public Object {
		DynamicArray<Thread> threads;

		Condition taskCondition;

		Mutex taskMutex;

		PackedQueue<Task> tasks;

		AtomicU32 taskCount;

		public:
		/**
		 * Constructs a new `Async` instance using `allocator` as the memory allocation strategy and
		 * `hardwarePriority` to determine how much of the hardware to use as a value between `0`
		 * and `1`.
		 */
		Async(Allocator * allocator, float hardwarePriority);

		~Async() override;

		/**
		 * Dispatches `task` asynchronously.
		 */
		void Execute(Task const & task);

		/**
		 * Waits for all currently executing tasks to complete.
		 *
		 * Note that attempting to wait for a task that never finishes will cause a deadlock on the
		 * waiting thread.
		 */
		void Wait();
	};
}
