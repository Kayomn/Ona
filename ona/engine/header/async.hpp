
namespace Ona::Engine {
	using namespace Ona::Collections;

	using Task = Callable<void()>;

	class Async : public Object {
		DynamicArray<ThreadHandle> threads;

		Condition * taskCondition;

		Mutex * taskMutex;

		PackedQueue<Task> tasks;

		AtomicU32 taskCount;

		public:
		Async(Allocator * allocator, float hardwarePriority);

		~Async() override;

		void Execute(Task const & task);

		void Wait();
	};
}
