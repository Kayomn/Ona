
namespace Ona::Engine {
	using Task = Callable<void()>;

	class ThreadServer : public Object {
		public:
		/**
		 * Executes `task` asynchronously.
		 */
		virtual void Execute(Task const & task) = 0;

		/**
		 * Initializes all worker threads and begins asynchronous processing of any pending tasks
		 * executed prior to, or after, being called.
		 *
		 * Returns `true` if the `ThreadServer` was started successfully, otherwise `false`.
		 */
		virtual bool Start() = 0;

		/**
		 * Kills all worker threads and halts the `ThreadServer`, pausing any task in the work
		 * queue.
		 */
		virtual void Stop() = 0;

		/**
		 * Halts the calling thread until all tasks running on the `ThreadServer` conclude
		 * execution.
		 */
		virtual void Wait() = 0;
	};

	/**
	 * Loads a platform-specific `ThreadServer` instance into memory.
	 *
	 * `hardwarePriority` specifies what percentage of access to concurrency hardware will be
	 * requested by the process as a value between `0` and `1`. A value of `1` or greater will try
	 * to request access to all concurrency hardware, and will likely cause context switching on any
	 * platform that implements its own process scheduler.
	 */
	ThreadServer * LoadThreadServer(float hardwarePriority);

	/**
	 * Attempts to load access to the operating system filesystem as a `FileServer`.
	 */
	FileServer * LoadFilesystem();

	/**
	 * Prints `message` to the operating system standard output.
	 */
	void Print(String const & message);
}
